
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

const int YEARS = -1;
const float INIT_BALANCE = 41499.20;
int TRADE_DAYS;


float b(Model m, float p, float h) {

  const float b = (m.a1 * p) + (m.b1 * h) + m.c1;  

  if(b < _bl || b > _bu) {
    printf("invalid value for b(p,h):%f(%f,%f):\n",b, p, h);
    print_model(m);
    exit(1);
  }

  return b;
}

float s(Model m) {
  return m.d2;
}

Trade_Execution calc_balance_fees(Trade_Execution exec) {

  const bool is_rec = exec.result == REC || exec.result == BREC;

  if(is_rec) exec.gain = exec.shares * exec.rec_price - exec.shares * exec.buy_price;
  else exec.gain = exec.shares * exec.sale_price - exec.shares * exec.buy_price;

  const float daily_interest = MARGIN_DPR * 0.5 * exec.shares * exec.buy_price;
  exec.interest = exec.held * daily_interest;
  
  float fees; 
  if(is_rec) {
    fees = exec.shares * exec.buy_price * AVG_TRANSATION_FEE + exec.shares * exec.rec_price * AVG_TRANSATION_FEE;
  } else {
    fees = exec.shares * exec.buy_price * AVG_TRANSATION_FEE + exec.shares * exec.sale_price * AVG_TRANSATION_FEE;
  }

  exec.balance = exec.balance + exec.gain - fees - exec.interest;

  return exec;
}

Trade_Execution sale_helper(int row, bool bm_sale_rec, float price, Trade_Execution exec, int init_sell_row) {

  exec.sell_date = date(row);

  if(bm_sale_rec) {

    price = max(close(row + 1), price);
    exec.held = init_sell_row - row;

  } else {

    price = max(open(row), price);
    exec.held = init_sell_row - row + 1;

  }

  if(exec.result == SALE || exec.result == BSALE) {
    exec.sale_price = price;
  } else {
    exec.rec_price = price;
  }

  exec.gain = (price - exec.buy_price) * (2.0f / exec.buy_price);

  return exec;

}

Trade_Execution execute_sale(Trade_Execution exec, int buy_row, Model m, bool bm_buy) {

  float stop_price = m.sl * exec.buy_price;

  const int init_sell_row = bm_buy ? buy_row : buy_row - 1;

  for(int row=init_sell_row; row >= 0; row--) {

    float prev_high = max_high(row + 1);

    float h = days_since_price(exec.buy_price, row, TRADE_DAYS);
    /*float delta = prev_high / buy_price;
    float wait = init_row - row;
    float s = m.a2*h + m.b2*delta + m.c2*wait + m.d2;*/
    exec.sale_price = s(m) * exec.buy_price;

    const bool no_bm_buy_today = !(bm_buy && row == buy_row);
    const bool bm_sale = no_bm_buy_today && bm_high(row) > exec.sale_price;
    const bool mkt_sale = mkt_high(row) > exec.sale_price;

    if(bm_sale || mkt_sale) {

      exec.result = bm_sale ? BSALE : SALE;
      
      exec = sale_helper(row, bm_sale, exec.sale_price, exec, init_sell_row);
      
      return calc_balance_fees(exec);

    } else if(no_bm_buy_today && bm_low(row) < stop_price || mkt_low(row) < stop_price) {

      exec.rec_price = m.r * exec.buy_price;

      const bool bm_stop = bm_low(row) < stop_price;

      const int stop_row = bm_stop ? row: row - 1; row;

      for(row = stop_row; row >= 0; row--) {

        const bool bm_rec = 
          !(bm_buy && row == buy_row) && // can't use no_bm_buy_today again because row's been incremented
          !(bm_stop && row == stop_row) && 
          bm_high(row) > exec.rec_price;
        
        const bool mkt_rec = mkt_high(row) > exec.rec_price;

        if(bm_rec || mkt_rec) {        

          exec.result = bm_rec ? BREC : REC;
          
          exec = sale_helper(row, bm_rec, exec.rec_price, exec, init_sell_row);
          
          return calc_balance_fees(exec);
        }
      }
    }
  }

  return exec;
}

Result run_limits(Model m, bool all_rows) {

  printf("optimal.c YEARS=%i INIT_BALANCE=%.2f Mode=%s\n", YEARS, INIT_BALANCE, all_rows ? "ALL" : "HTML");
  
  float normalized_return_sum = 0.00;
  int run_days = 0;
  int exec_cnt = 1; // last execution doesn't complete

  if(YEARS < 0) TRADE_DAYS = sizeof(AAPL) / sizeof(AAPL[0]);
  else TRADE_DAYS = floor(250 * YEARS);

  int first_row = TRADE_DAYS - 2;

  Trade_Execution exec;
  exec.balance = INIT_BALANCE;

  int miss_cnt = 0;

  for(int row=first_row; row >= 0; row--) {

const char* tmp = date(row);

    TradeState prev_exec_result = exec.result;
    const float prev_exec_balance = all_rows ? INIT_BALANCE : exec.balance;

    // TODO: reset everything in exec except {balance & result}
    exec.sell_date = "Unknown";
    exec.buy_price = -1;
    exec.rec_price = -1;
    exec.sale_price = -1;
    exec.shares = -1;
    //-----------------TODO

    float prev_close = close(row + 1); 
    float prev_low = mkt_low(row + 1);
    float prev_high = mkt_high(row + 1);

    float h = days_since_low(row);

    float p = 1 - (prev_high - prev_close) / (prev_high - prev_low);

    float b_factor = b(m, p, h);

    exec.buy_price = b_factor * prev_low;

    if(min_low(row) < exec.buy_price) {

      exec.buy_date = date(row);

      // this means the sale function is passed a bm_buy with no mods (same day)
      const bool bm_buy = 
        bm_low(row) < exec.buy_price &&
        !(miss_cnt == 0 && prev_exec_result == BREC) &&
        !(miss_cnt == 0 && prev_exec_result == BSALE);

      if(bm_buy) exec.buy_price = min(close(row + 1), exec.buy_price);
      else exec.buy_price = min(open(row), exec.buy_price);

      exec.shares = floor( (2 * prev_exec_balance) / exec.buy_price );

      exec = execute_sale(exec, row, m, bm_buy);
      exec_cnt++;

      const float normalize_return = (exec.balance - prev_exec_balance) / prev_exec_balance;

      normalized_return_sum += normalize_return;

      exec.nrs = normalized_return_sum;

      log_trade(exec);

      run_days += exec.held;

      // For comparison to HTML version
      if(!all_rows) {
// printf("\nbefore date:%s\n", date(row));
        row = row - exec.held;
        if(bm_buy) row++;
        
 
//printf("\nafter date:%s\n", date(row));
      }

        miss_cnt = 0;

    } else {
      // buy missed
      run_days++;
      miss_cnt++;
    }
  }

  const float daily_return = normalized_return_sum / run_days;
  const float year_return = 250 * daily_return;

  const float daily_exec = exec_cnt / run_days;
  const float year_exec = 250 * exec_cnt / run_days;

  Result results;
  results.n_return = year_return;
  results.exec_cnt = year_exec;

  return results;
}

void solo_run(Model m) {
  
  Result rst = run_limits(m, false);
  print_result(rst);
  print_model(m);

}

void explore() {

  int loops = 0;
  int vcnt = 0;

  Model min = {999, 999, 999, 999, 999, 999, 999, 999, 999, 999};
  Model max = {-999, -999, -999, -999, -999, -999, -999, -999, -999, -999};

  int complete = 0;

  // P=a1
  float span = -25; // 100 steps = 0.0833333
  for (double a1 = span; a1 < 0.00; a1 += 0.0001) {

    int progress = floor(100 * (1-(a1/span)));
    if(progress > complete) {
      complete = progress;
      printf("complete:%i%% \n", complete);
      //print_model(min);
      //print_model(max);
      print_time(); 
    }

    // H=b1
    for (double b1 = 0.0; b1 < 0.00090225; b1 += 0.0000001) {  // 100 steps = 0.000003
      
      for (double c1 = 0.88; c1 < 1.12; c1 += 0.001) {  // 100 steps = 0.0012
    

      //for (double d1 = 0.95; d1 < 0.99; d1 += 0.01) {
              
/*      for (double a2 = -0.5; a2 <= 1.15; a2 += 0.001) {  
          for (double b2 = -0.5; b2 <= 1.15; b2 += 0.001) {   
            for (double c2 = -0.5; c2 <= 1.15; c2 += 0.001) {   
              for (double d2 = -0.5; d2 <= 1.15; d2 += 0.01) {  
      }}}}}
              */

                    loops++;

                    Model m = {a1, b1, c1};  

                    if(valid(m)) {
                      vcnt++;

                      min = update_min(min, m);
                      max = update_max(max, m);
                    }
                }
              }
            }
     

  printf("[VALID COUNT]: %i\n", vcnt);
  printf("[TOTAL LOOPS]: %i\n\n\n", loops);
  printf("a1:{%.8f,%.8f}\n", min.a1, max.a1);
  printf("b1:{%.8f,%.8f}\n", min.b1, max.b1);
  printf("c1:{%.8f,%.8f}\n", min.c1, max.c1);

  /*
  
    a1:{-0.05000000,-0.01000000}
    b1:{0.00000000,0.00010000}
    c1:{0.97000000,1.01000000}

  */

}

int main() {


  Model m;
  m.a1 = -0.04360000;
  m.b1 = 0.00000200;
  m.c1 = 1.00400000;
  m.d2 = 1.02499998;
  m.sl = 0.985000;
  m.r = 1;

  solo_run(m);

  //explore();

  //brute_force();

}

void brute_force() {

  Result best_run = {-1, -1};
  Model best_m = {-1};

  Model min = {999, 999, 999, 999, 999, 999, 999, 999, 999, 999};
  Model max = {-999, -999, -999, -999, -999, -999, -999, -999, -999, -999};

  int complete = 0;

  const float span = -0.05;

  float a1 = -0.0041;
float b1 = 0.000172;

float c1 = 1.01;
float s = 1.025;
float sl = 0.985;
  //for (double a1 = span; a1 < -0.01; a1 += 0.0004) {

    int progress = floor(100 * (1-(a1/span)));
    if(progress > complete) {
      complete = progress;
      printf("complete:%i%%  ", complete);
      if(best_m.a1 != -1) {
        print_result(best_run);
        print_model(best_m);
      }
      print_time(); 
    }

    //for (double b1 = 0.0; b1 < 0.0001; b1 += 0.000001) {  

      //for (double c1 = 0.97; c1 < 1.01; c1 += 0.0004) {  

     //   for(float s = 1.01; s < 1.5; s += 0.005) {  

    //      for(float sl = 0.97; sl < 0.99; sl += 0.005) {
      float r = 1;
            //for(float r = 0.95; r < 1.20; r += 0.01) {

              Model m = {a1, b1, c1};
              m.d2 = s;
              m.sl = sl;
              m.r = r;  

              if(valid(m) && s > sl && s > r) {

                Result result = run_limits(m, true);
                
                if(result.n_return > best_run.n_return) {
                  best_run = result;
                  best_m = m;

                  min = update_min(min, m);
                  max = update_max(max, m);
                }
                
              }
              
           // }
       //   }
        //}
     // }
 //   }
  //}

  printf("\n[RANGE]\n");
  printf("a1:{%.8f,%.8f}\n", min.a1, max.a1);
  printf("b1:{%.8f,%.8f}\n", min.b1, max.b1);
  printf("c1:{%.8f,%.8f}\n", min.c1, max.c1);
  printf("s:{%.8f,%.8f}\n", min.d2, max.d2);
  printf("\n\n[BEST]\n");
  print_result(best_run);
  print_model(best_m);
}

/* NOTABLE RESULTS:
 *
 n_return:0.375551 exec_cnt:806
a1:-0.01000000, b1:0.00000000, c1:1.00960000, d1:0.00000000, a2:0.00000000, b2:0.00000000, c2:0.00000000, d2:1.49600339, sl:0.980000, r:1.000000
*
*/
