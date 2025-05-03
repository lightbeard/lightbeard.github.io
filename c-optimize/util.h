#include <time.h>
#include "AAPL.h"

// increased from 0.0583 just to be conservative, also compounding in trade days
const float MARGIN_DPR = 0.0593 / 250;
const float AVG_TRANSATION_FEE = 0.000499028440;

typedef enum { false, true } bool;

typedef enum {SALE, BSALE, REC, BREC} TradeState;

const char* TradeStates[] = {
  "SALE",
  "BSALE",
  "REC",
  "BREC"
};

typedef struct  {
  float n_return; int exec_cnt;
} Result;

typedef struct {
  TradeState result;
  char * buy_date;
  char * sell_date;
  float buy_price;
  int shares;
  float sale_price;
  float rec_price;
  int held;
  float gain;
  float interest;
  float nrs;
  double balance;
} Trade_Execution;

typedef struct {
  double a1; double b1; double c1; double d1;
  double a2; double b2; double c2; double d2;
  float sl; float r;
} Model;

const float _bu = 1.01;
const float _bl = 0.96;

struct Sample {
  int p;
  int h1;
  float h2;
  float w;
};

const struct Sample samples[] = {
  {1, 400},
  {0.5, 400},
  {0, 400},

  {1, 100},
  {0.5, 100},
  {0, 100},

  {1, 40},
  {0.5, 40},
  {0, 40},

  {1, 5},
  {0.5, 5},
  {0, 5},
  
  {1, 2},
  {0.5, 2},
  {0, 2},

  {1, 1},
  {0.5, 1},
  {0, 1}
};

bool valid(Model m) {

  double sum1 = m.a1 + m.b1;
  //double sum2 = m.a2 + m.b2 + m.c2;

  if(sum1 < 0.01 && sum1 > -0.01) return false;
  //if(sum2 < 0.005 && sum2 > -0.005) return false;

  for (int i = 0; i < sizeof(samples)/sizeof(samples[0]); i++) {
    float p = samples[i].p;  
    float h1 = samples[i].h1;
      
//      float h2 = samples[i].h2;
//      float z = samples[i].z;
//      float w = samples[i].w;

 //     float B = m.a1 * h1 + m.b1;
  //    if (B < _bl || B > _bu) {
    //    return false;
  //  }

    float B = (m.a1 * p) + (m.b1 * h1) + m.c1;

    if (B < _bl || B > _bu) {
        return false;
    }


//      float S = m.a2 * h2 + m.b2 * z + m.c2 * w + m.d2;

/*      if (B * S < _cl || B * S > _cu) {
          return false;
      }
          */
  }
  return true;
}


/*bool deep_validation(Model m, int trade_days) {

  for (int row = 0; row < trade_days - 2; row++) {
    float prev_close = close(row + 1); 
    float prev_low = mkt_low(row + 1);
    float prev_high = mkt_high(row + 1);

    float h = DAYS_SINCE_LOW[row];
    float p = 1 - (prev_high - prev_close) / (prev_high - prev_low);

    float B = (m.a1 * p) + (m.b1 * h) + m.c1;
    if (B < _bl || B > _bu) return false;
  }
  return true;
}*/

void log_trade(Trade_Execution exec) {
  printf("[%s] buy_date=%s sell_date=%s buy_price=%.2f shares=%i sale_price=%.2f rec_price=%.2f held=%i gain=%.2f interest=%.2f nrs=%.4f balance=%.2f\n",
    TradeStates[exec.result],
    exec.buy_date,
    exec.sell_date,
    exec.buy_price,
    exec.shares,
    exec.sale_price,
    exec.rec_price,
    exec.held,
    exec.gain,
    exec.interest,
    exec.nrs,
    exec.balance
  );

}

int days_since_low(int row) {
  return HISTORY[row][0];
}

int days_since_high(int row) {
  return HISTORY[row][1];
}

Model update_min(Model min, Model m) {

  if(m.a1 < min.a1) min.a1 = m.a1;
  if(m.b1 < min.b1) min.b1 = m.b1;
  if(m.c1 < min.c1) min.c1 = m.c1;

  return min;
}

Model update_max(Model max, Model m) {
  
  if(m.a1 > max.a1) max.a1 = m.a1;
  if(m.b1 > max.b1) max.b1 = m.b1;
  if(m.c1 > max.c1) max.c1 = m.c1;

  return max;
}

void print_model(Model m) {
  printf("a1:%.8f, b1:%.8f, c1:%.8f, d1:%.8f, a2:%.8f, b2:%.8f, c2:%.8f, d2:%.8f, sl:%f, r:%f\n", m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2, m.sl, m.r);
}

void print_result(Result r) {
  printf("n_return:%f exec_cnt:%i\n", r.n_return, r.exec_cnt);
}

void print_time() {
  time_t timer;
  char buffer[26];
  struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 26, "%H:%M:%S", tm_info);
  printf("%s\n", buffer);
}

// 0=date 1=close 2=open 3=mkt_high 4=mkt_low 5=bm_high 6=bm_low

const char* date(int row) {
  char* date = TRADE_DATES[row];
  return date;
}

float close(int row) {
  const float close = AAPL[row][0];
  return close;
}

float open(int row) {
  const float open = AAPL[row][1];
  return open;
}

float mkt_high(int row) {
  const float high = AAPL[row][2];
  return high;
}

float mkt_low(int row) {
  const float low = AAPL[row][3];
  return low;
}

float bm_high(int row) {
  const float high = AAPL[row][4];
  return high;
}

float bm_low(int row) {
  const float low = AAPL[row][5];
  return low;
}

#ifdef max
  #undef max
#endif

#ifdef min
  #undef min
#endif

float max(float a, float b) {
  return (a > b) ? a : b;
}

float min(float a, float b) {
  return (a < b) ? a : b;
}

float max_high(int row) {

  const float high = max(mkt_high(row), bm_high(row));
  return high;
}

float min_low(int row) {
  const float low = min(mkt_low(row), bm_low(row));
  return low;
}

int days_since_price(float price, int row, int period) {
  int days = 0;

  for(int r = row + 2; r < period; r++) {
      days++;
      float low = min_low(r);
      float high = max_high(r);
      if(low < price && high > price) break;
  }

  return days;
}
