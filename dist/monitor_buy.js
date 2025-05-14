import WebSocket from 'ws';
import { log } from './utils/log.js';
import { send_alert } from './utils/notifier.js';
import { b, session, CONID, ACCOUNT, TRAIL_AMT, TRAIL_OFFSET, PRICE_LOG_INTERVAL_MS } from './utils/env.js';
import { get, post, time, get_session, get_qty, has_position, get_state, sleep } from './utils/api.js';

(async () => {

  let last_log_time = 0;

  const state = get_state();
  const mkt_low = state.low;
  const buy_price = parseFloat((mkt_low * b).toFixed(2));

  log(`🚀 ### monitor_buy.js ###\n ### mkt_low=${mkt_low}, buy_price=${buy_price}`);

  if (get_session() === session.OVT) {
    // TODO implement OVT limit order edge case
    log("🌙 OVT detected — exiting.");
    process.exit(0);
  }

  if (await has_position()) {
    log(`📛 Position held — cant buy - exiting.`);
    process.exit(0);
  }

  const tickle = await get('/tickle');
  const session_token = tickle.session;  
  log(`🔑 Fetched session token ${session_token}`);

  // 🔌 Connect WebSocket with auth token
  const ws = new WebSocket('wss://localhost:5000/v1/api/ws', {
    headers: {
      Cookie: `api=${session_token}`
    }
  });

  log(`🔌 WebSocket object created for ${ws.url}`);

  ws.on('open', async () => {
    log('🔌 WebSocket on open() invoked');
    await sleep(3000);
    const msg = `smd+${CONID}+{"fields":["31"]}`;
    ws.send(msg)
    log(`▶️ subscribe ws message: ${msg}`);
  });

  ws.on('message', async (raw) => {

    const msg_obj = JSON.parse(raw);

    if(msg_obj.topic === `smd+${CONID}`) {

      const tick_price = parseFloat(msg_obj['31']);

      // log occasionally
      const now = Date.now();
      if (now - last_log_time >= PRICE_LOG_INTERVAL_MS) {
        log(`📩 on_message received: ${raw.toString()}`);
        log(`📉 tick_price: $${tick_price.toFixed(2)} | trigger/buy_price: $${buy_price.toFixed(2)}`);
        last_log_time = now;
      }

      if (tick_price <= buy_price) {

        log(`✅ Trigger! tick_price=$${tick_price} <= buy_price=${buy_price}. Checking position...`);

        if (await has_position()) {
          log(`📛 Position held — cant buy - exiting.`);
          process.exit(0);
        } else {
          log(`✅ Position is empty — proceeding to place order...`);
        }

        const qty = await get_qty();
        const trail_order_price = tick_price - 10.00;

        await post(`/iserver/account/${ACCOUNT}/orders`, {
          "orders": [  
            {  
              acctId: ACCOUNT,
              conidex: `${CONID}@SMART`, 
              orderType: "TRAILLMT", 
              side: "BUY",  
              tif: "DAY",
              price: trail_order_price,
              auxPrice: TRAIL_OFFSET,
              trailingType: "amt",
              trailingAmt: TRAIL_AMT,
              quantity: qty,
              outsideRTH: true,
              isSingleGroup: true
            },

            {
              conidex: `${CONID}@OVERNIGHT`,
              orderType: "LMT", 
              side: "BUY",
              price: tick_price,
              quantity: qty,
              tif: "OVT",
              isSingleGroup: true
            }
          ]
        });

        send_alert(`[${get_session()} ${time()}] BUY OCA qty ${qty} buy ${buy_price} tick ${tick_price}: TRAILLMT price ${trail_order_price} offset ${TRAIL_OFFSET} amt ${TRAIL_AMT} |OVT limit ${tick_price}`);

        ws.close();
        log(`✅ Our task is done - exiting buy_monitor.js`);
        process.exit(0);

      } 
    }
  });

  ws.on('close', () => log('🔌 WebSocket closed.'));
  ws.on('error', (err) => log(`❌ WebSocket error: ${err.message}`));
})();
