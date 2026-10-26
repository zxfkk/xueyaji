/* Streaming oscillometry prototype. Reject incomplete data; never invent a result.
 * Filters use actual dt. Two HP stages reject the DC offset of a falling ramp.
 * A complete positive-going crossing cycle supplies both extrema and one HR interval.
 */
#include "bp_algorithm.h"
#include <math.h>
#include <string.h>
static PulsePeak_t peaks[MAX_PULSE_PEAKS];
static uint16_t count;
static bool initialized,positive,cycle,overflow,timing_bad,blank_active;
static uint32_t last_tick,first_tick,cycle_at,blank_until,cycle_missing;
static float previous,hp1,hp2,lp,baseline,maximum,minimum,recent,pressure_integral;
static uint16_t cycle_samples;
static bool finite_value(float x) { return x==x && x>-100000.0f && x<100000.0f; }
void BP_Algorithm_Reset(void) {
 memset(peaks,0,sizeof(peaks)); count=0; initialized=false; positive=false; cycle=false;
 overflow=false; timing_bad=false; blank_active=false; recent=0; blank_until=0; cycle_missing=0; cycle_samples=0;
}
void BP_Algorithm_NotifyValvePulse(uint32_t tick) { blank_until=tick+STEP_WATER_HAMMER_BLANK_MS; blank_active=true; }
uint16_t BP_Algorithm_GetPeakCount(void) { return count; }
float BP_Algorithm_GetRecentPulseAmp(void) { return recent; }
void BP_Algorithm_PushSample(float pressure,uint32_t tick) {
 uint32_t elapsed; float dt,a,old_hp1;
 if (!finite_value(pressure)) { timing_bad=true; cycle=false; return; }
 if (!initialized) {
  initialized=true; first_tick=tick; last_tick=tick; previous=pressure;
  baseline=pressure; hp1=hp2=lp=0; return;
 }
 elapsed=tick-last_tick;
 if (!elapsed) return;
 last_tick=tick;
 if (elapsed>80UL) {
  /* No interpolation across a long acquisition gap. */
  timing_bad=true; cycle=false; positive=false; previous=pressure;
  hp1=hp2=lp=0; first_tick=tick; baseline=pressure; return;
 }
 dt=(float)elapsed*0.001f;
 baseline+=dt/(0.30f+dt)*(pressure-baseline);
 if (cycle) pressure_integral+=baseline*(float)elapsed;
 if (blank_active && (int32_t)(tick-blank_until)<0) {
  /* Remove commanded step from the filter input, account for masked time.
   * Short gaps are tolerated, not claimed to preserve the waveform exactly. */
  previous=pressure; cycle_missing+=elapsed; return;
 }
 blank_active=false;
 a=0.32f/(0.32f+dt); old_hp1=hp1;
 hp1=a*(hp1+pressure-previous); previous=pressure;
 a=0.65f/(0.65f+dt); hp2=a*(hp2+hp1-old_hp1);
 lp+=dt/(0.032f+dt)*(hp2-lp);
 if (tick-first_tick<2000UL) return;
 if (cycle) {
  if (lp>maximum) maximum=lp;
  if (lp<minimum) minimum=lp;
  cycle_samples++;
 }
 if (positive) {
  if (lp < -0.025f) positive=false;
 } else if (lp>0.025f) {
  uint32_t duration=tick-cycle_at;
  positive=true;
  if (cycle && duration>=300UL && duration<=1600UL &&
      cycle_missing*4UL<duration && cycle_samples>=12U) {
   float amp=maximum-minimum;
   if (amp>=0.10f && amp<=14.0f && baseline>=40.0f && baseline<=250.0f) {
    if (count==MAX_PULSE_PEAKS) overflow=true;
    else {
     peaks[count].cuff_pressure=pressure_integral/(float)duration;
     peaks[count].pulse_amp=amp; peaks[count].timestamp_ms=tick;
     count++; recent=amp;
    }
   }
  }
  cycle=true; cycle_at=tick; pressure_integral=0; maximum=minimum=lp; cycle_missing=0; cycle_samples=0;
 }
 if (cycle && tick-cycle_at>1800UL) { cycle=false; cycle_missing=0; }
}
static bool reject(BP_Result_t *r,uint8_t code) { r->err_code=code; return false; }
bool BP_Algorithm_Calculate(BP_Result_t *r) {
 float smooth[MAX_PULSE_PEAKS],intervals[MAX_PULSE_PEAKS];
 float max_amp=0, sbp=0,dbp=0,map,median,sum=0;
 uint16_t i,j,imax=0,n=0,accepted=0;
 bool high=false,low=false;
 if (!r) return false;
 memset(r,0,sizeof(*r));
 if (timing_bad) return reject(r,BP_ERR_TIMING);
 if (overflow) return reject(r,BP_ERR_COVERAGE);
 if (count<8U) return reject(r,BP_ERR_SIGNAL);
 if (peaks[0].cuff_pressure-peaks[count-1U].cuff_pressure<40.0f)
  return reject(r,BP_ERR_COVERAGE);
 for (i=0;i<count;i++) {
  smooth[i]=peaks[i].pulse_amp;
  if (i>0 && i<count-1U) smooth[i]=0.25f*peaks[i-1U].pulse_amp+
     0.5f*peaks[i].pulse_amp+0.25f*peaks[i+1U].pulse_amp;
  if (smooth[i]>max_amp) { max_amp=smooth[i]; imax=i; }
  if (i && peaks[i].cuff_pressure>peaks[i-1U].cuff_pressure+3.0f)
   return reject(r,BP_ERR_COVERAGE);
 }
 if (imax<2U || imax+2U>=count || max_amp<0.2f) return reject(r,BP_ERR_COVERAGE);
 map=peaks[imax].cuff_pressure;
 for (i=imax;i>0;i--) {
  float target=KSBP_RATIO*max_amp;
  if (smooth[i]>=target && smooth[i-1U]<=target && smooth[i]>smooth[i-1U]) {
   float ratio=(target-smooth[i-1U])/(smooth[i]-smooth[i-1U]);
   sbp=peaks[i-1U].cuff_pressure+ratio*(peaks[i].cuff_pressure-peaks[i-1U].cuff_pressure);
   high=true; break;
  }
 }
 for (i=imax;i+1U<count;i++) {
  float target=KDBP_RATIO*max_amp;
  if (smooth[i]>=target && smooth[i+1U]<=target && smooth[i]>smooth[i+1U]) {
   float ratio=(target-smooth[i+1U])/(smooth[i]-smooth[i+1U]);
   dbp=peaks[i+1U].cuff_pressure+ratio*(peaks[i].cuff_pressure-peaks[i+1U].cuff_pressure);
   low=true; break;
  }
 }
 if (!high || !low || !(sbp>map && map>dbp) || sbp-dbp<10.0f)
  return reject(r,BP_ERR_COVERAGE);
 for (i=1;i<count;i++) {
  uint32_t delta=peaks[i].timestamp_ms-peaks[i-1U].timestamp_ms;
  if (delta>=300UL && delta<=1600UL) intervals[n++]=(float)delta;
 }
 if (n<5U) return reject(r,BP_ERR_SIGNAL);
 for (i=1;i<n;i++) {
  float value=intervals[i]; j=i;
  while (j && intervals[j-1U]>value) { intervals[j]=intervals[j-1U]; j--; }
  intervals[j]=value;
 }
 median=intervals[n/2U];
 for (i=0;i<n;i++) if (fabsf(intervals[i]-median)<median*0.20f) { sum+=intervals[i]; accepted++; }
 if (accepted<5U || accepted*4U<n*3U) return reject(r,BP_ERR_SIGNAL);
 r->hr=(uint16_t)(60000.0f*(float)accepted/sum+0.5f);
 if (r->hr<40U || r->hr>180U) return reject(r,BP_ERR_SIGNAL);
 r->sbp=sbp; r->dbp=dbp; r->map=map; r->valid=true; return true;
}
