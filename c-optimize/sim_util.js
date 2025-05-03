

/*
0=Date      1=close 2=vol    3=open 4=high 5=low 6=bm_high 7=bm_low 
["4/25/2025",209.28,38222260,206.37,209.75,206.2,208.91,205.03],
*/

function date(row) {
    return AAPL[row][0];
}

function close(row) {
    return AAPL[row][1];
}

function open(row) {
    return AAPL[row][3];
}

function mkt_high(row) {
    return AAPL[row][4];
}   

function mkt_low(row) {
    return AAPL[row][5];
}

function bm_high(row) {
    return AAPL[row][6];
}

function bm_low(row) {
    return AAPL[row][7];
}

function min_low(row) {
    return Math.min(mkt_low, bm_low);
}

function max_high(row) {
    return Math.max(mkt_high, bm_high);
}

function days_since_low() {
    let days = 0;

    const prev_mkt_low = mkt_low(row + 1);
    
    for(let r = row + 2; r < AAPL.length; r++) {
        days++;
        let high = mkt_high(r);
        if(high < prev_mkt_low) break;
    }

    return days;
}

function days_since_buy_price() {
  let days = 0;

  if(typeof EXEC[EI].buy_price !== "number" ) throw new Error();

  for(let r = row + 2; r < AAPL.length; r++) {
      days++;
      let low = min_low(r);
      let high = max_high(r);
      if(low < EXEC[EI].buy_price && high > EXEC[EI].buy_price) break;
  }

  return days;
}
