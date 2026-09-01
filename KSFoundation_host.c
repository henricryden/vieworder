/*******************************************************************************************************
 * Neuro MR Physics group
 * Department of Neuroradiology
 * Karolinska University Hospital
 * Stockholm, Sweden
 *
 * Filename : KSFoundation_host.c
 *
 * Authors  : Stefan Skare, Enrico Avventi, Henric Rydén, Ola Norbeck
 * Date     : 2024-Oct-01
 *******************************************************************************************************/

/**
********************************************************************************************************
* @file KSFoundation_host.c
* @brief #### This file contains functions only accessible on HOST
********************************************************************************************************/

/* Epic CV's, variables and arrays */
extern int cfswgut;
extern int cfswrfut;
extern int cfsrmode;
extern int cfgcoiltype;
extern float GAM;

extern int rhkacq_uid;
extern int ks_plot_filefmt;
extern int ks_plot_kstmp;
extern char ks_psdname[256];
#if EPIC_RELEASE > 25
extern int cffield;
#endif

#if EPIC_RELEASE > 29
#include "PsdPath.h"
#endif

#include <KSFoundation_private.h>

#include <alloca.h>
#include <float.h>
#include <sys/stat.h>

#include <sys/types.h>
int ftruncate(int fd, off_t length);

/*******************************************************************************************************
 *  EPIC function prototypes
 *******************************************************************************************************/
STATUS inittargets(LOG_GRAD *lgrad, PHYS_GRAD *pgrad);
STATUS obloptimize_epi(LOG_GRAD *lgrad, PHYS_GRAD *pgrad, SCAN_INFO *scaninfotab, INT slquant, INT plane_type, INT coaxial, INT method, INT debug, INT *newgeo, INT srmode);
STATUS minseqrfamp(INT *Minseqrfamp, const INT numPulses, const RF_PULSE *rfpulse, const INT entry);
STATUS maxsar(INT *Maxseqsar, INT *Maxslicesar, DOUBLE *Avesar, DOUBLE *Cavesar, DOUBLE *Pksar, DOUBLE *B1rms, const INT numPulses, const RF_PULSE *rfpulse, const INT entry, const INT tr_val);
STATUS minseq( INT *p_minseqgrad, GRAD_PULSE *gradx, const INT gx_free, GRAD_PULSE *grady, const INT gy_free, GRAD_PULSE *gradz, const INT gz_free, const LOG_GRAD *log_grad, const INT seq_entry_index, const INT samp_rate, const INT min_tr, const INT e_flag, const INT debug_flag);


/*******************************************************************************************************
 *  Init
 *******************************************************************************************************/

void ks_init_read(KS_READ *read) {
  KS_READ defacq = KS_INIT_READ;
  *read = defacq;
}




void ks_init_trap(KS_TRAP *trap) {
  KS_TRAP deftrap = KS_INIT_TRAP;
  *trap = deftrap;
}




void ks_init_wait(KS_WAIT *wait) {
  KS_WAIT defwait = KS_INIT_WAIT;
  *wait = defwait;
}




void ks_init_wave(KS_WAVE *wave) {
  KS_WAVE defwave = KS_INIT_WAVE;
  *wave = defwave;
}




void ks_init_rf(KS_RF *rf) {
  KS_RF defrf = KS_INIT_RF;
  *rf = defrf;
}




void ks_init_sms_info(KS_SMS_INFO *sms_info) {
  KS_SMS_INFO defsms_info = KS_INIT_SMS_INFO;
  *sms_info = defsms_info;
}




void ks_init_selrf(KS_SELRF *selrf) {
  KS_SELRF defselrf = KS_INIT_SELRF;
  *selrf = defselrf;
}




void ks_init_readtrap(KS_READTRAP *readtrap) {
  KS_READTRAP defread = KS_INIT_READTRAP;
  *readtrap = defread;
}




void ks_init_phaser(KS_PHASER *phaser) {
  KS_PHASER defphaser = KS_INIT_PHASER;
  *phaser = defphaser;
}




void ks_init_epi(KS_EPI *epi) {
  KS_EPI defepi = KS_INIT_EPI;
  *epi = defepi;
}




void ks_init_readwave(KS_READWAVE* readwave) {
  *readwave = (KS_READWAVE)KS_INIT_READWAVE;
}




void ks_init_gradrfctrl(KS_GRADRFCTRL *gradrfctrl) {
  *gradrfctrl = (KS_GRADRFCTRL)KS_INIT_GRADRFCTRL;
}




void ks_init_seqcontrol(KS_SEQ_CONTROL *seqcontrol) {
  KS_SEQ_CONTROL defseqcontrol = KS_INIT_SEQ_CONTROL;
  *seqcontrol = defseqcontrol;
}




void ks_init_seqcollection(KS_SEQ_COLLECTION *seqcollection) {
  KS_SEQ_COLLECTION defseqcollection = KS_INIT_SEQ_COLLECTION;
  *seqcollection = defseqcollection;
}




STATUS ks_init_slewratecontrol(LOG_GRAD *loggrd, PHYS_GRAD *phygrd, float srfact) {

  if (fabs(srfact) < FLT_EPSILON) {
    return ks_error("%s: slewrate factor can not be zero", __FUNCTION__);
  }

  if (srfact < 1.0 && phygrd != NULL) {
    phygrd->xrt = (int) ((float) phygrd->xrt / srfact);
    phygrd->yrt = (int) ((float) phygrd->yrt / srfact);
    phygrd->zrt = (int) ((float) phygrd->zrt / srfact);
  }

  if (loggrd != NULL) {
    loggrd->xrt = (int) ((float) loggrd->xrt / srfact);
    loggrd->yrt = (int) ((float) loggrd->yrt / srfact);
    loggrd->zrt = (int) ((float) loggrd->zrt / srfact);
  }

  return SUCCESS;
}




void ks_init_echotrain(KS_ECHOTRAIN * const echotrain) {
  *echotrain = (KS_ECHOTRAIN)KS_INIT_ECHOTRAIN;
  int readout;
  int state;
  for (readout = 0; readout < KS_MAXUNIQUE_READ; readout++) {
    echotrain->controls[readout] = (KS_READCONTROL)KS_INIT_READ_CONTROL;
    for (state = 0; state < KS_READCONTROL_MAXSTATE; state++) {
      echotrain->controls[readout].state[state] = (KS_READCONTROL_STATE)KS_INIT_READCONTROL_STATE;
    }
  }
}




/*******************************************************************************************************
 *  Eval
 *******************************************************************************************************/

STATUS ks_eval_addtoseqcollection(KS_SEQ_COLLECTION *seqcollection, KS_SEQ_CONTROL *seqctrl) {
  int i;

  if (seqcollection == NULL) {
    return ks_error("%s: seqcollection is NULL", __FUNCTION__);
  }

  seqctrl->collection = seqcollection;

  if (seqctrl->duration == 0) {
    /* Return early if seqctrl.duration = 0 so we are not adding sequence modules with zero duration to the collection.
       This is in concert with:
       - not performing createseq() in KS_SEQLENGTH if seqctrl.duration = 0
       - not playing sequence modules in scan() using ks_scan_playsequence() if seqctrl.duration = 0

       It is the task of the PG function (e.g. <seqmodule>_pg()) of the current sequence module to set seqctrl.duration > 0 based on the waveform content in that module
       and <seqmodule>_pg() should therefore be called before this function.
       Preferred method is to add the following to the end of <seqmodule>_pg():
       #ifndef IPG
        // HOST only:
        ks_eval_seqctrl_setminduration(&seqctrl, tmploc.pos); // tmploc.pos now corresponds to the end of last gradient in the sequence
       #endif
    */
    return SUCCESS;
  }

  if (seqcollection->numseq > KS_MAXUNIQUE_SEQUENCES) {
    return ks_error("%s: Number of sequence modules has been exceeded (max: %d)", __FUNCTION__, KS_MAXUNIQUE_SEQUENCES);
  }

  if (seqcollection->mode == KS_SEQ_COLLECTION_LOCKED) {
    return ks_error("%s - (%s): seqcollection.mode = KS_SEQ_COLLECTION_LOCKED. Module must be added before GEReq_eval_rfscaling() and GEReq_eval_checkTR_SAR_calcs()", __FUNCTION__, seqctrl->description);
  }

  /* check for previous registrations of this sequence module.  If found, return */
  if (seqcollection->numseq > 0) {
    for (i = 0; i < seqcollection->numseq; i++) {
      if (seqcollection->seqctrlptr[i] == seqctrl)
        return SUCCESS;
    }
  }

  /* register the sequence module */
  seqcollection->seqctrlptr[seqcollection->numseq++] = seqctrl;

  return SUCCESS;
}




STATUS ks_eval_seqcollection_isadded(KS_SEQ_COLLECTION *seqcollection, KS_SEQ_CONTROL *seqctrl) {
  int i;

  if (seqctrl->duration == 0) {
    return SUCCESS; /* it is not added, but we should still not complain as .duration = 0
                       means that it won't be used anyway */
  }

  /* check for previous registrations of this sequence module.  If found, return */
  if (seqcollection->numseq > 0) {
    for (i = 0; i < seqcollection->numseq; i++) {
      if (seqcollection->seqctrlptr[i] == seqctrl)
        return SUCCESS;
    }
  }

  return FAILURE;

}




STATUS ks_eval_wait(KS_WAIT *wait, const char * const desc, int pg_duration, int max_duration) {

  ks_init_wait(wait);

  if (desc == NULL || desc[0] == ' ') {
    return ks_error("%s: desc (2nd arg) cannot be NULL or begin with a space", __FUNCTION__);
  } else {
    strncpy(wait->description, desc, KS_DESCRIPTION_LENGTH - 1);
  }


  wait->max_duration = max_duration;
  wait->pg_duration = pg_duration;

  /* initialize counters to 0 */
  wait->base.ninst = 0;
  wait->base.ngenerated = 0;
  wait->base.next = NULL;

  return SUCCESS;
}




STATUS ks_eval_isirot(KS_ISIROT *isirot, const char * const desc, int isinumber) {
  STATUS status;
  char tmpdesc[KS_DESCRIPTION_LENGTH];

  if ((isinumber < 4) || (isinumber > 7)) {
    return ks_error("%s: ISI number out of range (should be 4-7)", __FUNCTION__);
  }

  ks_create_suffixed_description(tmpdesc, desc, "_isirotfun");
  status = ks_eval_wait(&isirot->waitfun, tmpdesc, RUP_GRD(KS_ISI_time), RUP_GRD(KS_ISI_time));
  KS_RAISE(status);

  ks_create_suffixed_description(tmpdesc, desc, "_isirotupdate");
  status = ks_eval_wait(&isirot->waitrot, tmpdesc, RUP_GRD(KS_ISI_rotupdatetime), RUP_GRD(KS_ISI_time));
  KS_RAISE(status);

  isirot->isinumber = isinumber;
  isirot->duration = isirot->waitfun.max_duration + isirot->waitrot.max_duration;
  isirot->counter = 0;
  isirot->numinstances = 0;

  return SUCCESS;
}




STATUS ks_eval_read(KS_READ *read, const char * const desc) {
  char tmpdesc[KS_DESCRIPTION_LENGTH];
  int duration;
  float rbw;
  int tsp;
  int override_R1;
  int noutputs;

  /* store desired inputs before resetting */
  duration = read->duration;
  rbw = read->rbw;
  override_R1 = read->override_R1;

  strcpy(tmpdesc, desc); /* copy in case the read->description has been passed in before ks_init_read wipes it */

  /* Reset all fields (after saving the desired info) */
  ks_init_read(read);

  /* put back the input fields */
  read->duration = duration;
  read->rbw = ks_calc_nearestbw(rbw);  /* round to nearest valid rBW */
  read->override_R1 = override_R1;
  tsp = ks_calc_bw2tsp(read->rbw);
  strncpy(read->description, tmpdesc, KS_DESCRIPTION_LENGTH - 1);
  read->description[KS_DESCRIPTION_LENGTH - 1] = 0;

  /* don't allow empty description or a description with leading space */
  if (read->description == NULL || read->description[0] == ' ') {
    return ks_error("ks_eval_read: read name (2nd arg) cannot be NULL or leading space");
  }

  if (read->rbw <= 0) {
    return ks_error("ks_eval_read: field 'rbw' (1st arg) is not set");
  }
  if (read->duration <= 2) {
    return ks_error("ks_eval_read: field 'duration' (1st arg) must be a positive number in [us]");
  }

  noutputs = CEIL_DIV(read->duration, tsp);
  read->duration = noutputs * tsp;

  /* calculate read->filter (FILTER_INFO) */
  STATUS status = ks_calc_filter(&read->filt, tsp, read->duration);
  KS_RAISE(status);

  return SUCCESS;
}




STATUS ks_eval_trap_constrained(KS_TRAP *trap, const char * const desc, float ampmax, float slewrate, int minduration) {
  double triangletrapezoid_area;
  double maxramp_area;
  double requested_area;
  int maxramp_duration;
  int sign_reqarea;
  int minplateautime = KS_MINPLATEAUTIME;
  char tmpdesc[KS_DESCRIPTION_LENGTH];

  /* store desired inputs before resetting the trap object */
  requested_area = (double) trap->area;
  strcpy(tmpdesc, desc); /* copy in case the trap->description has been passed in before ks_init_trap wipes it */

  /* Reset all fields and waveforms (after saving the desired area) */
  ks_init_trap(trap);

  /* put back the input fields */
  trap->area = requested_area;

  strncpy(trap->description, tmpdesc, KS_DESCRIPTION_LENGTH - 1);
  trap->description[KS_DESCRIPTION_LENGTH - 1] = 0;

  /* don't allow empty description or a description with leading space */
  if (trap->description == NULL || trap->description[0] == ' ') {
    return ks_error("ks_eval_trap: trap name (2nd arg) cannot be NULL or leading space");
  }

  if (areSame(requested_area, 0)) {
    return SUCCESS;
  }

  /* sign control: Store the sign of the requested area. Ignore all other signs */
  sign_reqarea = (requested_area < 0) ? -1 : 1;
  requested_area = fabs(requested_area);
  slewrate = fabs(slewrate);
  ampmax = fabs(ampmax);

  /* make sure the minimum duration is divisible by 8us */
  minduration = RUP_FACTOR(minduration, 2 * GRAD_UPDATE_TIME);


  /* area for one ramp from zero to max amplitude ('ampmax') */
  maxramp_duration = RUP_GRD(ceil((double) ampmax / (double) slewrate)); /* [usec] */
  maxramp_area = (double) ampmax * ((double) maxramp_duration) / 2.0; /* [G/cm * usec] */

  /* area for a trapezoid with a plateau time of 'minplateautime' and maximum amplitude ('ampmax') */
  triangletrapezoid_area = ((double) minplateautime * (double) ampmax) + (2.0 * maxramp_area);

  if (requested_area > triangletrapezoid_area) {
    /*      ________......ampmax
           /        \
          /          \
         /            \
      __/              \__

      We need max target amplitude and use a plateau time > minplateautime to reach required area. [amp = ampmax] */

    trap->ramptime = maxramp_duration;
    trap->plateautime = (int) ceil((requested_area - 2.0 * maxramp_area) / (double) ampmax);

    /* Round *up* to nearest 8us to make the plateau contain an even # of points (which is always nice) */
    trap->plateautime = RUP_FACTOR(trap->plateautime, 2 * GRAD_UPDATE_TIME);


  } else {
    /*
      ......... ampmax

          _ amp
         / \
        /   \
     __/     \__

      We don't need max amplitude and will use a minimum plateau time of minplateautime
      [amp = ramptime * slewrate] */

    trap->plateautime = minplateautime; /* Our minimum plateau time */

    /* Quadratic solution in 'ramptime':
       slewrate*(ramptime)^2 + slewrate*minplateautime*(ramptime) - requested_area = 0
       ...where: ramptime = unknownamp / slewrate <=> unknownamp = ramptime * slewrate   */
    trap->ramptime = ((int) ceil(pow(requested_area / (double) slewrate + ((double) minplateautime * (double) minplateautime) / 4.0, 0.5))) - minplateautime / 2;

    /* Round *up* to nearest GRAD_UPDATE_TIME (unit: usec) */
    trap->ramptime = RUP_GRD(trap->ramptime);
  }


  /* amp (G/cm). Make the sign of the amp equal to that of the requested_area */
  trap->amp = (float) (sign_reqarea * requested_area / ((double) (trap->ramptime + trap->plateautime)));

  /* amp watchdog */
  if (fabs(trap->amp) > ampmax)
    return ks_error("ks_eval_trap: amp (%.3f) of trap '%s' exceeds ampmax (%.3f)", trap->amp, trap->description, ampmax);

  /* store the duration */
  trap->duration = trap->ramptime * 2 + trap->plateautime;

  /* if the duration is smaller than the minimum duration, increase the ramp times and reduce the gradient amp accordingly */
  if (trap->duration < minduration) {
    trap->ramptime += (minduration - trap->duration)/2;
    trap->duration = trap->ramptime * 2 + trap->plateautime;
    trap->amp = (float) (sign_reqarea * requested_area / ((double) (trap->ramptime + trap->plateautime)));
  }


  return SUCCESS;

}




STATUS ks_eval_trap(KS_TRAP *trap, const char * const desc) {

  return ks_eval_trap_constrained(trap, desc, ks_syslimits_ampmax(loggrd), ks_syslimits_slewrate(loggrd), 0);

}




STATUS ks_eval_trap2(KS_TRAP *trap, const char * const desc) {

  return ks_eval_trap_constrained(trap, desc, ks_syslimits_ampmax2(loggrd), ks_syslimits_slewrate2(loggrd), 0);

}




STATUS ks_eval_trap1(KS_TRAP *trap, const char * const desc) {

  return ks_eval_trap_constrained(trap, desc, ks_syslimits_ampmax1(loggrd), ks_syslimits_slewrate1(loggrd), 0);

}




STATUS ks_eval_trap_rotation_invariant_75_max_slewrate(KS_TRAP *trap, const char * const desc) {
  const float max_slewrate = 75.0/10000; /* 75 T/m/s converted to G/cm/us */
  return ks_eval_trap_constrained(trap, desc,
                                  ks_syslimits_ampmax_phys()/sqrt(3),
                                  FMin(3, ks_syslimits_slewrate_phys()/sqrt(3), ks_syslimits_slewrate(loggrd), max_slewrate),
                                  0);
}




STATUS ks_eval_trap_constrained_time_maxarea(KS_TRAP *trap, const char * const desc, float ampmax, float slewrate, float maxarea, int derate_slewrate) {
  int requested_time;
  int maxramp_duration;
  char tmpdesc[KS_DESCRIPTION_LENGTH];
  int sign = (maxarea < 0) ? -1 : 1;
  strcpy(tmpdesc, desc); /* copy in case the trap->description has been passed in before ks_init_trap wipes it */

  /* make sure the minimum duration is divisible by 8us */
  requested_time = RDN_FACTOR(trap->duration, 2 * GRAD_UPDATE_TIME);

  if (trap->duration < 4 || areSame(maxarea,0.0)) {
    ks_dbg("%s: dur:%d, area:%f", __FUNCTION__, trap->duration, maxarea);
    ks_init_trap(trap);
    return SUCCESS; 
  }

  /* Reset all fields and waveforms (after saving the desired area) */
  ks_init_trap(trap);
  strncpy(trap->description, tmpdesc, KS_DESCRIPTION_LENGTH - 1);
  trap->description[KS_DESCRIPTION_LENGTH - 1] = 0;

  /* don't allow empty description or a description with leading space */
  if (trap->description == NULL || trap->description[0] == ' ') {
    return ks_error("%s: trap name (2nd arg) cannot be NULL or leading space", __FUNCTION__);
  }
  if (requested_time <= 0) {
    return SUCCESS;
  }

  slewrate = fabs(slewrate); /* [(G/cm) / us] */
  ampmax = fabs(ampmax); /* [G/cm] */
  maxramp_duration = RUP_GRD(ceil((double) ampmax / (double) slewrate)); /* [usec] */
  if ((2*maxramp_duration + KS_MINPLATEAUTIME) > requested_time) {
    /* Not enough time to make it to the plateau */
    trap->plateautime = KS_MINPLATEAUTIME;
    trap->ramptime = (requested_time - KS_MINPLATEAUTIME) / 2;
    trap->amp = slewrate * trap->ramptime;
  } else {
    /* Enough time to reach plateau */
    trap->amp = ampmax;
    trap->ramptime = maxramp_duration;
    trap->plateautime = (requested_time - 2 * trap->ramptime);
  }

  trap->area = (trap->plateautime + trap->ramptime) * trap->amp * sign;
  if (fabs(trap->area) > fabs(maxarea)) {
    if (derate_slewrate == 1) {
      trap->amp = maxarea / (trap->plateautime + trap->ramptime);
    } else {
      trap->ramptime = RUP_GRD( (int) ((requested_time - sqrtf(requested_time*requested_time - 4*maxarea/slewrate)) / 2.0));
      trap->amp = trap->ramptime * slewrate;
      trap->plateautime = (requested_time - 2 * trap->ramptime);
    }
    /* Adjust the amplitude to never go above maxarea */
    trap->amp *= fabs(maxarea / ((trap->plateautime + trap->ramptime) * trap->amp));
    trap->area = (trap->plateautime + trap->ramptime) * trap->amp * sign;
  }

  trap->duration = trap->ramptime * 2 + trap->plateautime;
  if (trap->duration != requested_time) {
    ks_dbg("%s: Duration mismatch. this should not happen", __FUNCTION__);
  }
  return SUCCESS;
}




STATUS ks_eval_set_asym_padding(struct _readtrap_s *readtrap, float wanted_paddingarea_pre, float wanted_paddingarea_post) {
  if (readtrap->rampsampling) {
    return ks_error("%s: No support for rampsampled readouts with asymmetric padding", __FUNCTION__);
  }
  float previous_paddingarea = readtrap->paddingarea;
  float amp = readtrap->grad.amp;
  float ramp_area = readtrap->grad.ramptime * amp/ 2.0;

  float actual_pre_area = isNotSet(wanted_paddingarea_pre)   ? previous_paddingarea : FMax(2, ramp_area, wanted_paddingarea_pre);
  float actual_post_area = isNotSet(wanted_paddingarea_post) ? previous_paddingarea : FMax(2, ramp_area, wanted_paddingarea_post);

  int plateau_delay_pre  = (int)((actual_pre_area  - ramp_area)/ amp);
  int plateau_delay_post = (int)((actual_post_area - ramp_area)/ amp);

  readtrap->grad.plateautime = RUP_GRD(readtrap->acq.duration + plateau_delay_pre + plateau_delay_post);
  readtrap->acqdelay = RUP_FACTOR(readtrap->grad.ramptime + plateau_delay_pre, 2);

  /* Update convinient fields */
  readtrap->area2center += actual_pre_area - previous_paddingarea - ramp_area;
  readtrap->paddingarea = actual_pre_area - ramp_area;
  readtrap->grad.duration = RUP_GRD(2*readtrap->grad.ramptime + readtrap->grad.plateautime);
  readtrap->time2center = RUP_GRD((int) ((readtrap->area2center - ramp_area) / amp + readtrap->grad.ramptime));
  return SUCCESS;
}




STATUS ks_eval_trap1p(KS_TRAP *trap, const char * const desc) {

  return ks_eval_trap_constrained(trap, desc, ks_syslimits_ampmax1p(loggrd), ks_syslimits_slewrate1p(loggrd), 0);

}




STATUS ks_eval_ramp(KS_WAVE* ramp, float slew, float end_amp, int dwell) {
  float ramp_dur = KS_RUP_GRD_FLOAT(fabs(end_amp) / slew);

  float t[3] = {0.0};
  float G[3] = {0.0};

  /* simple ramp case */
  int act_dur = RUP_FACTOR((int)(ramp_dur), (int)(2 * dwell));
  G[2] = end_amp;
  t[1] = act_dur - ramp_dur;
  t[2] = act_dur;

  return ks_eval_coords2wave(ramp, t, G, 3, dwell, "ramp");
}




/* G, t needs to be preallocated. At maximum 5 entries are used. */
STATUS ks_calc_unconstrained_transition(float *G, float *t, int *num_points,
                                        float req_area, float end_amp,
                                        float slew, float ampmax, unsigned int min_plateau) {

  if (end_amp < 0 || ampmax <= 0 || slew <= 0) {
    return KS_THROW("Negative entries are not supported( end_amp = %.2f, ampmax = %.2f, slew = %.2f)", end_amp, ampmax, slew);
  }
  if (end_amp > ampmax) {
    return KS_THROW("ampmax > end_amp (%f > %f)", ampmax, end_amp);
  }
  if (!G || !t || !num_points) {
    return KS_THROW("NULL inputs");
  }

  const float fast_ramp_dur = KS_RUP_GRD_FLOAT(end_amp / slew);
  const float fast_ramp_area = end_amp * fast_ramp_dur / 2.0;

  if (req_area <= fast_ramp_area) {
    G[0] = 0.0f;
    t[0] = 0.0f;
    G[1] = end_amp;
    t[1] = fast_ramp_dur;
    *num_points = 2;
    return SUCCESS;
  }

  /* Assume an extra plateau strategy and calculate the extra time */
  const float extra_plateau_time = (req_area - fast_ramp_area) / end_amp;
  if (extra_plateau_time <= min_plateau) {
    t[3] = KS_RUP_GRD_FLOAT(fast_ramp_dur + extra_plateau_time);
    t[2] = t[3] - extra_plateau_time;
    t[1] = t[2] - fast_ramp_dur;
    t[0] = 0;

    G[3] = end_amp;
    G[2] = end_amp;
    G[1] = 0.0f;
    G[0] = 0.0f;

    *num_points = 4;
    return SUCCESS;
  }

  /* first calculate area for maxamp and minimum plateau */
  float plat_dur = min_plateau;
  float plat_amp = ampmax;
  float rampdown_dur = (plat_amp - end_amp) / slew;
  const float simple_ramp_area = 0.5 * end_amp * end_amp / slew; /* Not raster aligned */
  float rampup_dur = plat_amp / slew;
  const float area =   plat_dur * plat_amp 
                     + rampup_dur * plat_amp
                     - simple_ramp_area;

  if (area < req_area) {
    /* prolong plateau */
    plat_dur += (req_area - area) / plat_amp;
  } else {
    /* Use minimum plat_dur and keep max slew */
    plat_amp = (-plat_dur * slew + sqrt( plat_dur*plat_dur*slew*slew + 2*end_amp*end_amp + 4*req_area*slew )) / 2 ;
    rampdown_dur = (plat_amp - end_amp) / slew;
    rampup_dur = plat_amp / slew;

  }
  if (rampdown_dur < 0) {
    return KS_THROW("Ops");
  }

  t[4] = KS_RUP_GRD_FLOAT(rampup_dur + plat_dur + rampdown_dur);
  t[3] = t[4] - rampdown_dur;
  t[2] = t[3] - plat_dur;
  t[1] = t[2] - rampup_dur;
  t[0] = 0.0f;

  G[4] = end_amp;
  G[3] = plat_amp;
  G[2] = plat_amp;
  G[1] = 0.0f;
  G[0] = 0.0f;

  *num_points = 5;

  return SUCCESS;

} /* ks_calc_unconstrained_transition() */




/* G, t needs to be preallocated. At maximum 6 entries are used. */
STATUS ks_calc_exact_transition(float *G, float *t, int *num_points,
                                float req_area, float end_amp, float min_duration,
                                float slew, float ampmax, unsigned int min_plateau) {

  STATUS status;

  if (end_amp < 0 || ampmax <= 0 || slew <= 0) {
    return KS_THROW("Negative entries are not supported( end_amp = %.2f, ampmax = %.2f, slew = %.2f)", end_amp, ampmax, slew);
  }
  if (!G || !t || !num_points) {
    return KS_THROW("NULL inputs");
  }

  min_duration = KS_RUP_GRD_FLOAT(min_duration);

  const float fast_ramp_dur = KS_RUP_GRD_FLOAT(end_amp / slew);
  const float fast_ramp_area = end_amp * fast_ramp_dur / 2.0;

  /* Fast ramp + plateau with duration at least equal to min_duration */
  const float ramp_plat_dur = min_duration > fast_ramp_dur ? min_duration : fast_ramp_dur;
  const float ramp_plat_area = fast_ramp_area + (ramp_plat_dur - fast_ramp_dur) * end_amp;

  if (req_area >= ramp_plat_area) {

    /* Batwing is needed */
    status = ks_calc_unconstrained_transition(G, t, num_points, 
                                              req_area, end_amp,
                                              slew, ampmax, min_plateau);
    KS_RAISE(status);

    return SUCCESS;
  }

  const float plateau_dur = 2*req_area/end_amp - min_duration;
  if (plateau_dur > min_plateau) {

    /* ramp + plateau */
    t[0] = 0.0f;
    t[1] = min_duration - plateau_dur;
    t[2] = min_duration;

    G[0] = 0.0f;
    G[1] = end_amp;
    G[2] = end_amp;

    *num_points = 3;

    return SUCCESS;
  }

  if (req_area >= fast_ramp_area) {

    /* Shorter ramp */
    const float ramp_dur = 2*req_area/end_amp;
    t[2] = KS_RUP_GRD_FLOAT(ramp_dur);
    t[1] = t[2] - ramp_dur;
    t[0] = 0;

    G[2] = end_amp;
    G[1] = 0;
    G[0] = 0;

    *num_points = 3;

    return SUCCESS;
  }

  /* A negative blip is needed */
  const float blip_area = fast_ramp_area - req_area;
  float ramp_dur = ampmax/slew;
  float plat_dur = min_plateau;
  const float maxamp_minplat_area = (plat_dur + ramp_dur) * ampmax;
  if (blip_area < maxamp_minplat_area) {
    ramp_dur = sqrt(plat_dur*plat_dur/4.0 + blip_area/slew) - plat_dur / 2.0;
  } else {
    plat_dur += (blip_area - maxamp_minplat_area) / ampmax;
  }

  t[5] = KS_RUP_GRD_FLOAT(fast_ramp_dur + plat_dur + 2*ramp_dur);
  t[4] = t[5] - fast_ramp_dur;
  t[3] = t[4] - ramp_dur;
  t[2] = t[3] - plat_dur;
  t[1] = t[2] - ramp_dur;
  t[0] = 0;

  G[5] = end_amp;
  G[4] = 0;
  G[3] = -blip_area / (plat_dur + ramp_dur);
  G[2] = G[3];
  G[1] = 0;
  G[0] = 0;

  /* TODO: dilate the ramp and the blip is there is extra time? */

  *num_points = 6;

  return SUCCESS;
}




/* G, t needs to be preallocated. At maximum 6 entries are used. */
STATUS ks_calc_minslew_transition(float *G, float *t, int *num_points,
                                  float req_area, float end_amp, float min_duration,
                                  float slew, float ampmax, unsigned int min_plateau) {

  STATUS status;

  if (end_amp < 0 || ampmax <= 0 || slew <= 0) {
    return KS_THROW("Negative entries are not supported( end_amp = %.2f, ampmax = %.2f, slew = %.2f)", end_amp, ampmax, slew);
  }
  if (!G || !t || !num_points) {
    return KS_THROW("NULL inputs");
  }

  min_duration = KS_RUP_GRD_FLOAT(min_duration);
  const float fast_ramp_dur = KS_RUP_GRD_FLOAT(end_amp / slew);
  if (min_duration < fast_ramp_dur) {
    /* There is no available time to reduce slew */
    min_duration = fast_ramp_dur;
  }

  const float slow_ramp_area = end_amp * min_duration / 2.0;

  if (req_area <= slow_ramp_area) {
    G[0] = 0.0f;
    t[0] = 0.0f;
    G[1] = end_amp;
    t[1] = min_duration;
    *num_points = 2;
    return SUCCESS;    
  }

  status = ks_calc_exact_transition(G, t, num_points,
                                    req_area, end_amp, min_duration,
                                    slew, ampmax, min_plateau);
  KS_RAISE(status);

  return SUCCESS;
}




/* G, t needs to be preallocated. At maximum 5 entries are used. */
STATUS ks_calc_maxarea_transition(float *G, float *t, int *num_points,
                                  float req_area, float end_amp, float max_duration,
                                  float slew, float ampmax, unsigned int min_plateau) {

  STATUS status;

  /* Generate the fastest strategy first to see if there is any extra time */
  status = ks_calc_unconstrained_transition(G, t, num_points,
                                            req_area, end_amp,
                                            slew, ampmax, min_plateau);
  KS_RAISE(status);

  if (t[(*num_points) -1] >= max_duration) {
    return SUCCESS;
  }

  max_duration = KS_RUP_GRD_FLOAT(max_duration);

  const float fast_ramp_dur = KS_RUP_GRD_FLOAT(end_amp / slew);

  if (max_duration < fast_ramp_dur + min_plateau) {

    /* ramp + plateau */
    t[0] = 0;
    t[1] = fast_ramp_dur;
    t[2] = max_duration;

    G[0] = 0;
    G[1] = end_amp;
    G[2] = end_amp;

    *num_points = 3;


    return SUCCESS;
  }

  /* Batwing */
  const float max_rampup_dur = ampmax / slew;
  const float ramp_dur = (max_duration - fast_ramp_dur - min_plateau)/2;
  if ( (fast_ramp_dur + ramp_dur) > max_rampup_dur) {
    /* Reached ampmax, add as much plateau time as possible */
    float rampdown_dur = (ampmax - end_amp) /slew;
    t[0] = 0;
    t[1] = max_rampup_dur;
    t[2] = max_duration - rampdown_dur;
    t[3] = max_duration;

    G[0] = 0;
    G[1] = ampmax;
    G[2] = ampmax;
    G[3] = end_amp;
    *num_points = 4;

    return SUCCESS;
  }

  /* Batwing with min plateau */
  t[0] = 0;
  t[1] = fast_ramp_dur + ramp_dur;
  t[2] = t[1] + min_plateau;
  t[3] = max_duration;

  G[0] = 0;
  G[1] = t[1] * slew;
  G[2] = G[1];
  G[3] = end_amp;

  *num_points = 4;


  return SUCCESS;
}




STATUS ks_eval_crusher(KS_WAVE* crusher, const float end_amp, const ks_crusher_constraints crusher_constraints) {

  STATUS status;

  float G[6] = {0};
  float t[6] = {0}; 
  int num_points;

  const float amp_sign = end_amp >= 0 ? 1 : -1;

  switch(crusher_constraints.strategy) {
  case KS_CRUSHER_STRATEGY_EXACT:
    status = ks_calc_exact_transition(G, t, &num_points,
                                      amp_sign*crusher_constraints.area, amp_sign*end_amp,
                                      crusher_constraints.min_duration,
                                      crusher_constraints.slew, crusher_constraints.ampmax,
                                      crusher_constraints.min_plateau);
    break;
  case KS_CRUSHER_STRATEGY_MIN_SLEW:
    status = ks_calc_minslew_transition(G, t, &num_points,
                                        amp_sign*crusher_constraints.area, amp_sign*end_amp,
                                        crusher_constraints.min_duration,
                                        crusher_constraints.slew, crusher_constraints.ampmax,
                                        crusher_constraints.min_plateau);
    break;
  case KS_CRUSHER_STRATEGY_MAX_AREA:
    status = ks_calc_maxarea_transition(G, t, &num_points,
                                        amp_sign*crusher_constraints.area, amp_sign*end_amp,
                                        crusher_constraints.min_duration,
                                        crusher_constraints.slew, crusher_constraints.ampmax,
                                        crusher_constraints.min_plateau);
    break;
  case KS_CRUSHER_STRATEGY_PLATEAU:
    status = ks_calc_unconstrained_transition(G, t, &num_points,
                                              amp_sign*crusher_constraints.area, amp_sign*end_amp,
                                              crusher_constraints.slew, amp_sign*end_amp,
                                              crusher_constraints.min_plateau);
    break;
    default:
    status = ks_calc_unconstrained_transition(G, t, &num_points,
                                              amp_sign*crusher_constraints.area, amp_sign*end_amp,
                                              crusher_constraints.slew, crusher_constraints.ampmax,
                                              crusher_constraints.min_plateau);
  }
  KS_RAISE(status);
  
  

  /* Flip coordinates if the sign was negative */
  int i=0;
  for (; i<num_points; ++i) {
    G[i] *= amp_sign;
  }

  for (i = 1; i < num_points-1; ++i) {
    float const delta_t = t[i] - t[i-1];
    if (delta_t < 0.0f) {
      ks_dbg("Time point %d is earlier than the previous time point (%f < %f)", i, t[i], t[i-1]);
      /* Adjust if smaller than half a GRAD_UPDATE_TIME */
      if (-delta_t < (GRAD_UPDATE_TIME/2.0)) {
        ks_dbg("Adjusting time point %d to %f", i, t[i-1]);
        t[i] = t[i-1];
      }
      
    }
  }

  status = ks_eval_coords2wave(crusher, t, G, num_points, GRAD_UPDATE_TIME, "crusher");
  if (status != SUCCESS) {
    ks_dbg("strategy: %d", crusher_constraints.strategy);
    ks_dbg("area: %f", crusher_constraints.area);
    ks_dbg("end_amp: %f", end_amp);
    ks_dbg("min_duration: %d", crusher_constraints.min_duration);
    ks_dbg("min_plateau: %d", crusher_constraints.min_plateau);
    ks_dbg("slew: %f", crusher_constraints.slew);
    for (i=0; i<num_points; ++i) {
      ks_dbg("%f: %f", t[i], G[i]);
    }
  }
  KS_RAISE(status);

  return SUCCESS;
}




STATUS ks_eval_readtrap_constrained(KS_READTRAP *readtrap, const char * const desc, float ampmax, float slewrate) {
  float minfov;
  int tsp = 0;
  float readpixelarea;
  int nreadpixels;
  float nonacq_readarea;
  float ampmax_kspace_speedlimit;
  STATUS status;

  /* This function is for setting up readout trapezoid with fixed amplitude based on FOV and rBW */

  if (desc == NULL || desc[0] == ' ') {
    return ks_error("ks_eval_readtrap: desc (2nd arg) cannot be NULL");
  }
  if (areSame(readtrap->acq.rbw, 0) || isNotSet(readtrap->acq.rbw)) {
    return ks_error("ks_eval_readtrap: field 'acq.rbw' (1st arg) is not set");
  }

  /* reset KS_TRAP's */
  ks_init_trap(&readtrap->grad);
  ks_init_wave(&readtrap->omega);


  /* round rBW to nearest valid value */
  readtrap->acq.rbw = ks_calc_nearestbw(readtrap->acq.rbw);
  tsp = ks_calc_bw2tsp(readtrap->acq.rbw); /* us per sample point */

  minfov = ks_calc_minfov(ampmax, tsp);

  /* Programmer's error */
  if (readtrap->res % 2 || readtrap->res <= 0 || readtrap->res > 2048) {
    return ks_error("ks_eval_readtrap(%s): field 'res' must be even and in the range 2-2048", desc);
  }
  if (readtrap->fov < 10 || readtrap->fov > 600) {
    return ks_error("ks_eval_readtrap(%s): field 'fov' must be in the range 10-600 mm", desc);
  }
  if (abs(readtrap->nover) > readtrap->res / 2) {
    return ks_error("ks_eval_readtrap(%s): field 'nover' may not exceed res/2", desc);
  }
  if (readtrap->acq.rbw > 250) {
    return ks_error("ks_eval_readtrap(%s): rBW cannot exceed +/- 250 kHz/FOV", desc);
  }
  /* Operator's error */
  if (readtrap->fov < minfov && readtrap->rampsampling == 0) {
    return ks_error("%s: Please increase the FOV to %.1f [cm] or decrease the rBW", desc, minfov / 10.0);
  }

  if (abs(readtrap->nover)) {
    nreadpixels = readtrap->res / 2 + abs(readtrap->nover);
  } else {
    nreadpixels = readtrap->res;
  }
  if (nreadpixels % 2) {
    return ks_error("ks_eval_readtrap(%s): number of sampling points must be even (%d)", desc, nreadpixels);
  }


  if (readtrap->rampsampling == 1) { /* rampsampling */

    /* Gradient area necessary to move one pixel in k-space in the read direction */
    readpixelarea = ks_calc_fov2gradareapixel(readtrap->fov); /* [(G/cm)*usec] */

    /* Speed limit the readout (i.e. put a cap on the readout amplitude) based on the FOV and the chosen rBW */
    ampmax_kspace_speedlimit = FMin(2, readpixelarea / tsp, ampmax);

    /* delay of ACQ start relative to the beginning of the attack ramp.
       For rampsampled cases we need to ensure the XTR pulses of
       consecutive readouts don't overlap eg. EPI train */
    int min_acqdelay = (int)(XTRSETLNG + XTR_TAIL + 1) / 2;
    if (readtrap->acqdelay < min_acqdelay) {
      readtrap->acqdelay = min_acqdelay;
    }
    readtrap->acqdelay = RUP_FACTOR(readtrap->acqdelay, 2);

    /* gradient area before and after the acq window */
    nonacq_readarea = slewrate /* [G/cm/usec] */ * (readtrap->acqdelay) * (readtrap->acqdelay) /* [usec^2] */; /* from both sides of the readout */

    /* required gradient area: gradient area needed during the acq window + nonacq_readarea */
    readtrap->grad.area = (readpixelarea * nreadpixels) + nonacq_readarea;

    status =ks_eval_trap_constrained(&readtrap->grad, desc, ampmax_kspace_speedlimit, slewrate, 0);
    KS_RAISE(status);


    if (readtrap->grad.ramptime > readtrap->acqdelay) {
      /* rampsampling attempt ok, continue calculating area2center and time2center */

      if (readtrap->nover > 0) { /* partial Fourier on the first part of the readout */
        readtrap->area2center = readpixelarea * readtrap->nover + nonacq_readarea / 2;
      } else { /* Full Fourier, or partial Fourier on the last part of the readout */
        readtrap->area2center = readpixelarea * readtrap->res / 2 + nonacq_readarea / 2;
      }

      /* area = (s*t^2)/2 [G/cm/usec] * [usec^2] */
      if (readtrap->area2center < slewrate * (readtrap->grad.ramptime * readtrap->grad.ramptime / 2.0)) {
        /* k-space center is on the attack ramp */
        readtrap->time2center = (int) sqrt(readtrap->area2center * 2.0 / slewrate);
      } else {
        float arealeftonplateau = readtrap->area2center - (readtrap->grad.ramptime * readtrap->grad.amp / 2.0);
        readtrap->time2center = (int) readtrap->grad.ramptime + (arealeftonplateau / readtrap->grad.amp); /* whole ramp + some plateau time */
      }

      /* round up the readout window time to the nearest multiple of '2*tsp' to always make filt.outputs even */
      readtrap->acq.duration = RUP_FACTOR(readtrap->grad.duration - readtrap->acqdelay * 2, (int) (2 * tsp));

      /* update 'acqdelay' due to this potential roundoff */
      readtrap->acqdelay = (readtrap->grad.duration - readtrap->acq.duration) / 2;

    } else {

      readtrap->rampsampling = 0; /* let's skip ramp sampling then (and be caught by the next non-rampsampled case) */

    }

  } /* rampsampling */




  if (readtrap->rampsampling == 0) { /* No rampsampling */

    if (desc[0] == ' ') {
      ks_init_trap(&readtrap->grad);
      return ks_error("ks_eval_readtrap: desc (2nd arg) cannot begin with a space");
    } else {
      strncpy(readtrap->grad.description, desc, KS_DESCRIPTION_LENGTH - 1);
      readtrap->grad.description[KS_DESCRIPTION_LENGTH - 1] = 0;
    }

    /* Error if requested constraints cannot be fullfilled */
    if (readtrap->fov < minfov) {
      return ks_error("%s: %s - Please increase the FOV to %.1f [cm] or decrease the rBW", __FUNCTION__, desc, minfov / 10.0);
    }

    readtrap->grad.amp = ((float) (1.0 / (GAM * tsp * 1e-6) * (10.0 / readtrap->fov))); /* [G/cm] */
    readtrap->grad.ramptime = RUP_GRD(readtrap->grad.amp / slewrate); /* [usec] */
    readtrap->acq.duration = RUP_GRD(nreadpixels * tsp);

    /* adjust plateau time to account for padding */
    const float ramparea = readtrap->grad.ramptime * readtrap->grad.amp / 2.0;
    const int extradelay = RUP_GRD(FMax(2, readtrap->paddingarea - ramparea, 0.0) / readtrap->grad.amp);
    readtrap->grad.plateautime = readtrap->acq.duration + 2*extradelay; /* [usec] */ /* Nkx * dwell time */

    readtrap->grad.duration = readtrap->grad.plateautime + readtrap->grad.ramptime * 2; /* [usec] */
    readtrap->grad.area = (readtrap->grad.plateautime + readtrap->grad.ramptime) * readtrap->grad.amp; /* rounding effects */

    /* ACQ starts after the attack ramp */
    readtrap->acqdelay = readtrap->grad.ramptime + extradelay;

    const int pixelstocenter = readtrap->nover > 0 ?
      readtrap->nover : /* partial Fourier on the first part of the readout */
      readtrap->res/2;  /* Full Fourier, or partial Fourier on the last part of the readout */

    readtrap->area2center = (readtrap->grad.ramptime/2 + tsp * pixelstocenter + extradelay)  * readtrap->grad.amp;
    readtrap->time2center = readtrap->grad.ramptime + tsp * pixelstocenter + extradelay;

  } /* no rampsampling */

  /* round up to nearest multiple of GRAD_UPDATE_TIME (4us) */
  readtrap->time2center = RUP_GRD(readtrap->time2center);

  /* setup acq window */
  /* readtrap->acq.rbw is set > 0 by the calling function */
  KS_DESCRIPTION read_description;
  ks_create_suffixed_description(read_description, readtrap->grad.description, ".echo");
  status = ks_eval_read(&readtrap->acq, read_description);
  KS_RAISE(status);

  /* for rampsampling, create an omega waveform (for FOV shifts in the freq (read) direction for rampsampling) */
  if (readtrap->rampsampling) {
    float ramp_sample_period = readtrap->grad.ramptime - readtrap->acqdelay;
    float t[4] = {0.0f, ramp_sample_period, (float)readtrap->acq.duration - ramp_sample_period, (float)readtrap->acq.duration};
    float start_amp = readtrap->grad.amp / readtrap->grad.ramptime * readtrap->acqdelay; 
    float G[4] = {start_amp, readtrap->grad.amp, readtrap->grad.amp, start_amp};
    int k;
    /*ensure 150 mm fov offset is possible*/
    for (k = 0; k < 4; k++) G[k] *= GAM / 10.0; 
    KS_DESCRIPTION omega_description;
    ks_create_suffixed_description(omega_description, readtrap->grad.description, ".omega");
    STATUS status;
    status = ks_eval_coords2wave(&readtrap->omega, t, G, 4, GRAD_UPDATE_TIME, omega_description);
    KS_RAISE(status);
    readtrap->omega.fs_factor = 0.1; /* so 1 bit of iamp = 0.1 mm */
  } else {
    readtrap->omega.duration = 0;
  }


  return SUCCESS;
} /* ks_eval_readtrap_constrained() */


/*-*/

STATUS ks_eval_readtrap(KS_READTRAP *readtrap, const char * const desc) {

  return ks_eval_readtrap_constrained(readtrap, desc,
                                      ks_syslimits_ampmax(loggrd), ks_syslimits_slewrate(loggrd));

}




STATUS ks_eval_readtrap2(KS_READTRAP *readtrap, const char * const desc) {

  return ks_eval_readtrap_constrained(readtrap, desc,
                                      ks_syslimits_ampmax2(loggrd), ks_syslimits_slewrate2(loggrd));

}




STATUS ks_eval_readtrap1(KS_READTRAP *readtrap, const char * const desc) {

  return ks_eval_readtrap_constrained(readtrap, desc,
                                      ks_syslimits_ampmax1(loggrd), ks_syslimits_slewrate1(loggrd));

}




STATUS ks_eval_readtrap_constrained_sample_duration(struct _readtrap_s *readtrap,
                                                 const char* const desc,
                                                 const float ampmax,
                                                 const float slewrate,
                                                 const int t_A,
                                                 const int sample_duration, /* Should include t_A */
                                                 const float area_post,
                                                 const float area_total) {
  int nreadpixels;
  float amp;
  int t_Sr_pre = 0;
  int t_Sp = 0;
  int t_Sr_post = 0;
  int t_Ep = 0;
  int t_Er = 0;
  int time_to_center = 0;
  /* reset KS_TRAP's */
  ks_init_trap(&readtrap->grad);
  ks_init_wave(&readtrap->omega);

  /* Description */
  strncpy(readtrap->grad.description, desc, KS_DESCRIPTION_LENGTH - 1);

  /* Programmer's error */
  if (readtrap->res % 2 || readtrap->res <= 0 || readtrap->res > 2048) {
    return ks_error("ks_eval_readtrap(%s): field 'res' must be even and in the range 2-2048", desc);
  }
  if (readtrap->fov < 10 || readtrap->fov > 600) {
    return ks_error("ks_eval_readtrap(%s): field 'fov' must be in the range 10-600 mm", desc);
  }
  if (abs(readtrap->nover) > readtrap->res / 2) {
    return ks_error("ks_eval_readtrap(%s): field 'nover' may not exceed res/2", desc);
  }
  if (abs(readtrap->nover)) {
    nreadpixels = readtrap->res / 2 + abs(readtrap->nover);
  } else {
    nreadpixels = readtrap->res;
  }
  if (nreadpixels % 2) {
    return ks_error("ks_eval_readtrap(%s): number of sampling points must be even (%d)", desc, nreadpixels);
  }

  float readpixelarea = ks_calc_fov2gradareapixel(readtrap->fov);

  /* First assume that both ramps can be used for data sampling. */
  /* delay of ACQ start relative to the beginning of the attack ramp.
    For rampsampled cases, this value should be at least one dwell time (tsp) */

  /* A is the ramp area where no sampling occurs */
  float A = 0.5 * slewrate * t_A * t_A;

  /* Calculate the required trail time, i.e. the time from end of sampling until end of gradient */
  int t_ErA = sqrt(2*area_post/slewrate);

  /* tE_r is the time from end of sampling until start of A */
  t_Er = t_ErA - t_A;

  /* Total duration */
  int t_total = sample_duration + t_ErA;

  /* Sample area = B + C */
  /* Sample area to center is B plus the part of C up to k-space center */
  float sample_area = readpixelarea * nreadpixels; /* G/cm * us*/
  float area_to_center = A + readpixelarea * abs(readtrap->nover); /* Assumes center-out sampling */

  /* The total area is known, as well as the duration and slewrate. Calculate the ramp time */
  int t_ramp = (t_total - sqrtf(t_total*t_total - 4.0 * area_total/slewrate)) / 2.0;
  t_ramp = RUP_FACTOR(t_ramp, 4);
 /* Check the dual ramp sampling assumption */
  if (t_ErA > t_ramp) {
 /* There is not enough ramp area to cover the required area_post.
    Recalculate t_ramp as there will only be one ramp sampled side: */
    t_Sr_pre = sample_duration - t_A - sqrtf( (slewrate*(sample_duration*sample_duration - t_A*t_A) -2*sample_area)/ slewrate);
    if (t_Sr_pre < 0) { return ks_error("Ops"); }
    t_ramp = RUP_FACTOR(t_A + t_Sr_pre, 4);
    t_Sr_pre = t_ramp - t_A;
    amp = slewrate * t_ramp;
    float area_ramp = t_ramp * amp / 2;
    float area_plateau = area_total - 2*area_ramp;
    int plateau_time = RUP_FACTOR( (int) (area_plateau / amp), 4);
    t_Sp = sample_duration - t_ramp; 
    t_Ep = (area_post - area_ramp) / amp;

    readtrap->grad.ramptime = t_ramp;
    readtrap->grad.plateautime = plateau_time;


    /* Doesn't matter */
    t_Sr_post = 0;
    t_Er = t_Sr_pre;
    t_ErA = t_ramp; /* Doesn't matter */
    t_total = 2*t_ramp + t_Sp + t_Ep; /* Doesn't matter */

    ks_error("%s: Single ramp sample case", __FUNCTION__);
    /* Calculate time to center */
    if (area_ramp > area_to_center) { /* Echo occurs on ramp up */
      time_to_center = sqrtf(2*area_to_center/slewrate);
      if (time_to_center < t_A) {
        return ks_error("%s: Echo occurs before sampling (ramptime: %.2f, time_to_center %.2f)", __FUNCTION__, t_A/1000.0f, time_to_center/1000.0f);
      }
    } else { /* Echo occurs on plateau */
      float plateau_time_to_center = (area_to_center - area_ramp) / amp;
      time_to_center = t_ramp + plateau_time_to_center;
    }
  } else { /* Dual ramp sampling holds */
    /* Double ramp sampled case - No extra area on plateau */
    ks_error("%s: Double ramp sample case", __FUNCTION__);
    amp = slewrate * t_ramp;
    float area_ramp = t_ramp * amp / 2;
    float area_plateau = area_total - 2*area_ramp;
    t_Sp = RUP_FACTOR( (int) (area_plateau / amp), 4);
    t_Sr_pre = t_ramp - t_A;
    t_Sr_post = t_ramp - t_A - t_Er;
    t_Ep = 0;
    readtrap->grad.ramptime = RUP_FACTOR(t_ramp, 4);
    readtrap->grad.plateautime = RUP_FACTOR(t_Sp + t_Ep, 4);
    /* t_Er already set */

    area_plateau = t_Sp * amp;
    if (area_ramp > area_to_center) { /* Echo occurs on ramp up */
      time_to_center = sqrtf(2*area_to_center/slewrate);
      if (time_to_center < t_A) {
        return ks_error("%s: Echo occurs before sampling (ramptime: %.2f, time_to_center %.2f)", __FUNCTION__, t_A/1000.0f, time_to_center/1000.0f);
      }
    } else if (area_to_center <= (area_ramp + area_plateau)) { /* Echo occurs on plateau */
      float plateau_time_to_center = (area_to_center - area_ramp) / amp;
      time_to_center = t_ramp + plateau_time_to_center;
    } else if (area_to_center <= (2*area_ramp + area_plateau)) { /* Echo occurs on ramp down */
      float rem_area = area_to_center - area_plateau - area_ramp;
      float time_to_center_from_ramp_down = amp/slewrate - sqrtf( powf(amp/slewrate,2) - 2*rem_area/slewrate);
      time_to_center = t_ramp + t_Sp + time_to_center_from_ramp_down;
    } else {
      return ks_error("%s: There is no echo occurring on this waveform", __FUNCTION__);
    }
  }

  if (amp > ampmax) {
    return ks_error("%s: Amp limit exceeded (%.2f > %.2f)", __FUNCTION__, amp, ampmax);
  }

  /* Setup object properties */
  readtrap->acqdelay = t_A;
  readtrap->grad.amp = amp;
  readtrap->grad.duration = 2*readtrap->grad.ramptime + readtrap->grad.plateautime;
  readtrap->grad.area = (readtrap->grad.ramptime + readtrap->grad.plateautime) * amp;

  float dwelltime = 1.0/(4257.59 * readtrap->fov * 0.1 * amp * 0.000001); 
  dwelltime = 2.0;
  readtrap->acq.rbw = ks_calc_tsp2bw((int)dwelltime);
  dwelltime = ks_calc_bw2tsp(readtrap->acq.rbw);
  readtrap->area2center = area_to_center;
  readtrap->time2center = time_to_center;
  readtrap->acq.duration = RUP_FACTOR(t_Sr_pre + t_Sp + t_Sr_post, (int) (2 * dwelltime));
  if (ks_eval_read(&readtrap->acq, "read") == SUCCESS) ;
  /* Acquisition window */
  /* if (ks_eval_read(&readtrap->acq, "echo") == FAILURE) { return FAILURE; } */

  /* Omega board*/
  float ramp_sample_period = readtrap->grad.ramptime - readtrap->acqdelay;
  float t[4] = {0.0f, ramp_sample_period, (float)readtrap->acq.duration - ramp_sample_period, (float)readtrap->acq.duration};
  float start_amp = readtrap->grad.amp / readtrap->grad.ramptime * readtrap->acqdelay; 
  float G[4] = {start_amp, readtrap->grad.amp, readtrap->grad.amp, start_amp};
  int k;
  /*ensure 150 mm fov offset is possible*/
  for (k = 0; k < 4; k++) {
   G[k] *= GAM / 10.0;
  }
  KS_DESCRIPTION omega_description;
  ks_create_suffixed_description(omega_description, readtrap->grad.description, ".omega");
  STATUS status;
  status = ks_eval_coords2wave(&readtrap->omega, t, G, 4, GRAD_UPDATE_TIME, omega_description);
  KS_RAISE(status);
  readtrap->omega.fs_factor = 0.1; /* so 1 bit of iamp = 0.1 mm */

  return SUCCESS;
}




STATUS ks_eval_readwave_constrained(KS_READWAVE* readwave,
                                    KS_WAVE* acq_waves,
                                    const int numstates,
                                    int flag_symmetric_padding,
                                    ks_crusher_constraints pre_crusher_constraints,
                                    ks_crusher_constraints post_crusher_constraints) {
  STATUS status;

  typedef struct max {
    float amp;
    int index;
  } max;
  if (acq_waves == NULL) {
    return KS_THROW("acq_waves is NULL");
  }
  int i = 0;
  KS_WAVE *precrushers  = (KS_WAVE*)malloc(numstates*sizeof(KS_WAVE));
  KS_WAVE *postcrushers = (KS_WAVE*)malloc(numstates*sizeof(KS_WAVE));
  int max_precrusher_res = 0;
  int max_postcrusher_res = 0;

  max acq_max = {0, -1};
  max acq_start_max = {0, -1};
  max acq_end_max = {0, -1};

  int acq_res = acq_waves[0].res;
  int acq_duration = acq_waves[0].duration;

  /* Programmer's error */
  if (acq_res <= 0) {
    return ks_error("%s (%s): invalid res (%d)", __FUNCTION__, acq_waves[i].description, acq_res);
  }

  if (readwave->fov < 10 || readwave->fov > 600) {
    return ks_error("%s (%s): field 'fov' must be in the range 10-600 mm", __FUNCTION__, acq_waves[i].description);
  }
  if (readwave->res % 2 || readwave->res <= 0 || readwave->res > 2048) {
    return ks_error("%s (%s): field 'res' must be even and in the range 2-2048", __FUNCTION__, acq_waves[i].description);
  }
  if (abs(readwave->nover) > readwave->res / 2) {
    return ks_error("%s (%s): field 'nover' may not exceed res/2", __FUNCTION__, acq_waves[i].description);
  }
/*   if (abs(readwave->nover) % 2) {
    return ks_error("%s (%s): nover must be even (%d)", __FUNCTION__, acq_waves[i].description, abs(readwave->nover));
  } */
  if ( (acq_waves[i].duration / acq_waves[i].res) != GRAD_UPDATE_TIME) {
    return ks_error("%s: acq_waves[i] must have dwell = GRAD_UPDATE_TIME", __FUNCTION__);
  }
  int polarity[numstates];
  for (; i < numstates; i++) {
    polarity[i] = 1;
    if ( acq_waves[i].res != acq_res ) {
      return ks_error("%s: acq_waves[%d] does not match res of acq_waves[0] (%d != %d)", __FUNCTION__, i, acq_waves[i].res, acq_waves[0].res);
    }
    if (acq_waves[i].duration != acq_duration) {
      return ks_error("%s: acq_waves[%d] does not match duration of acq_waves[0] (%d != %d)", __FUNCTION__, i, acq_waves[i].duration, acq_waves[0].duration);
    }
    if (acq_waves[i].waveform[0] < 0 && acq_waves[i].waveform[acq_waves[i].res - 1] < 0) {
      /* Assume entire wave is negative. Negate the wave, continue, and re-negate the state later */
      polarity[i] = -1;
      ks_wave_multiplyval(&acq_waves[i], -1.0);
    } else if (acq_waves[i].waveform[acq_waves[i].res - 1] < 0 || acq_waves[i].waveform[0] < 0){
      return ks_error("%s: acq_waves[%d] changes polarity", __FUNCTION__, i);
    }
    if (acq_waves[i].abs_max_amp > acq_max.amp) {
       acq_max.amp = acq_waves[i].abs_max_amp;
       acq_max.index = i;
    }

    const float start_amp = 1.5f * acq_waves[i].waveform[0] - 0.5f * acq_waves[i].waveform[1];
    if (acq_start_max.amp < start_amp) {
      acq_start_max.amp = start_amp;
      acq_start_max.index = i;
    }

    const float end_amp = 1.5f * acq_waves[i].waveform[acq_waves[i].res - 1] - 0.5f * acq_waves[i].waveform[acq_waves[i].res - 2];
    if (acq_end_max.amp < end_amp) {
      acq_end_max.amp = end_amp;
      acq_end_max.index = i;
    }
  }

  /* Set ampmax and slew if no preference */
  for (i = 0; i < 2; i++) {
    ks_crusher_constraints* constraints = i == 0 ? &pre_crusher_constraints
                                                 : &post_crusher_constraints;
    constraints->ampmax = constraints->ampmax > 0 ? constraints->ampmax : ks_syslimits_ampmax(loggrd);
    constraints->slew = constraints->slew > 0 ? constraints->slew : ks_syslimits_slewrate(loggrd);
  }

  max grad_max = acq_max;

  KS_WAVE max_precrusher = KS_INIT_WAVE;
  KS_WAVE max_postcrusher = KS_INIT_WAVE;
  status = ks_eval_crusher(&max_precrusher, acq_start_max.amp, pre_crusher_constraints);
  KS_RAISE(status);
  status = ks_eval_crusher(&max_postcrusher, acq_end_max.amp, post_crusher_constraints);
  KS_RAISE(status);

  for (i=0; i < numstates; i++) {  

    const float start_amp = 1.5f * acq_waves[i].waveform[0] - 0.5f * acq_waves[i].waveform[1];
    const float end_amp = 1.5f * acq_waves[i].waveform[acq_waves[i].res - 1] - 0.5f * acq_waves[i].waveform[acq_waves[i].res - 2]; 

    const float max_crusher_area = fabs(max_precrusher.area) > fabs(max_postcrusher.area) ?
      max_precrusher.area : max_postcrusher.area;

    pre_crusher_constraints.area = flag_symmetric_padding ? max_crusher_area : max_precrusher.area;
    pre_crusher_constraints.strategy = KS_CRUSHER_STRATEGY_EXACT;
    status = ks_eval_crusher(&precrushers[i], start_amp, pre_crusher_constraints);
    KS_RAISE(status);

    post_crusher_constraints.area = flag_symmetric_padding ? max_crusher_area : max_postcrusher.area;
    post_crusher_constraints.strategy = KS_CRUSHER_STRATEGY_EXACT;
    status = ks_eval_crusher(&postcrushers[i], end_amp, post_crusher_constraints);
    KS_RAISE(status);


    if (precrushers[i].abs_max_amp > grad_max.amp) {
      grad_max.amp = precrushers[i].abs_max_amp;
      grad_max.index = i;
    }
    if (postcrushers[i].abs_max_amp > grad_max.amp) {
      grad_max.amp = precrushers[i].abs_max_amp;
      grad_max.index = i;
    }

    status = ks_eval_mirrorwave(&postcrushers[i]);
    KS_RAISE(status);
    max_precrusher_res = IMax(2, max_precrusher_res, precrushers[i].res);
    max_postcrusher_res = IMax(2, max_postcrusher_res, postcrushers[i].res);
  }

  /* zeropad crushers to make them equally long */
  float* pad_array = (float*)calloc(KS_MAXWAVELEN/2, sizeof(float));
  if (pad_array == NULL) {
    return KS_THROW("Allocation failed");
  }
  
  int res = 0;
  for (i=0; i < numstates; i++) {
    readwave->grad.p_waveformstates[i] = readwave->grad_states[i];
    readwave->omega.p_waveformstates[i] = readwave->omega_states[i];
    float* dest_grad = readwave->grad.p_waveformstates[i];
    float* dest_omega = readwave->omega.p_waveformstates[i];
    res = 0;
    int res_diff_pre = max_precrusher_res - precrushers[i].res;
    int res_diff_post = max_postcrusher_res - postcrushers[i].res;

    status = ks_eval_append_two_waveforms(dest_grad, pad_array, 0, res_diff_pre);
    KS_RAISE(status);
    res += res_diff_pre;
    status = ks_eval_append_two_waveforms(dest_grad, precrushers[i].waveform, res, precrushers[i].res);
    KS_RAISE(status);
    res += precrushers[i].res;
    status = ks_eval_append_two_waveforms(dest_grad, acq_waves[i].waveform, res, acq_res);
    KS_RAISE(status);
    res += acq_res;

    status = ks_eval_append_two_waveforms(dest_omega, acq_waves[i].waveform, 0, acq_res);
    KS_RAISE(status);
    ks_waveform_multiplyval(dest_omega, GAM / 10.0, acq_res); /* [Gauss / cm] -> [Hz / mm] */

    status = ks_eval_append_two_waveforms(dest_grad, postcrushers[i].waveform, res, postcrushers[i].res);
    KS_RAISE(status);
    res += postcrushers[i].res;
    status = ks_eval_append_two_waveforms(dest_grad, pad_array, res, res_diff_post);
    KS_RAISE(status);
    res += res_diff_post;
  }
  free(pad_array);

  /* Eval the biggest wave (maxamp) in readwave->grad */
  /* copy the waveform */
  KS_DESCRIPTION description;
  memcpy(readwave->grad.waveform, readwave->grad.p_waveformstates[grad_max.index], sizeof(float) * res);
  ks_create_suffixed_description(description, acq_waves[grad_max.index].description, ".grad");

  strcpy(readwave->grad.description, description);
  readwave->grad.res = res;
  readwave->grad.duration = res * GRAD_UPDATE_TIME;


  ks_wave_compute_params(&readwave->grad);


  float sample_area2center;
  if (readwave->nover > 0) {
    /* nover > 0 results in early echo */
    sample_area2center = readwave->nover * ks_calc_fov2gradareapixel(readwave->fov);
  } else {
    sample_area2center = (readwave->res/2) * ks_calc_fov2gradareapixel(readwave->fov);
  }
  readwave->acqdelay = max_precrusher_res * GRAD_UPDATE_TIME;
  readwave->area2center = precrushers[acq_start_max.index].area + sample_area2center;
  readwave->time2center = ks_wave_time2area(&readwave->grad, readwave->area2center);
  if (readwave->time2center == KS_NOTSET) {
    return ks_error("%s: The read wave can't get to the center!",  __FUNCTION__);
  }

  ks_create_suffixed_description(description, acq_waves[acq_max.index].description, ".omega");


  memcpy(readwave->omega.waveform, readwave->omega.p_waveformstates[acq_max.index], sizeof(float) * res);
  strcpy(readwave->omega.description, description);
  readwave->omega.res = acq_res;
  readwave->omega.duration = acq_res * GRAD_UPDATE_TIME;

  ks_wave_compute_params(&readwave->omega);

  readwave->omega.fs_factor = 0.1; /* so 1 bit of iamp = 0.1 mm */

  readwave->acq.duration = acq_duration;
  if (readwave->acq.rbw <= 0.0) {
    const int sysmintsp = 2;
    float dwell = 1.0 / (GAM * acq_waves[acq_max.index].abs_max_amp * 1e-6) * (10.0 / readwave->fov);
    int tsp = (int) (dwell);
    tsp -= tsp % sysmintsp;
    for (; tsp >= sysmintsp; tsp -= sysmintsp) {
      if((acq_duration % (2 * tsp)) == 0) {
        break; /* duration must be even multiple of tsp */
      }
    }
    if (tsp < sysmintsp) {
      return ks_error("%s: readwave amplitude exceeds bw required to sample this fov", __FUNCTION__);
    }
    readwave->acq.rbw = ks_calc_tsp2bw(tsp);
  }
  ks_create_suffixed_description(description, acq_waves[0].description, ".readout");
  status = ks_eval_read(&readwave->acq, description);
  KS_RAISE(status);

  /* Setup the resampler parameters for the read object */
  readwave->acq.resampler.ndims = 1;
  readwave->acq.resampler.xwave = &readwave->grad;
  readwave->acq.resampler.wave_st_idx = readwave->acqdelay / GRAD_UPDATE_TIME;
  readwave->acq.resampler.target_res = readwave->nover ? readwave->res/2 + fabs(readwave->nover)
                                                       : readwave->res;
  readwave->acq.resampler.samples2center = (readwave->time2center - readwave->acqdelay) / readwave->acq.filt.tsp;

  /* Re-negate the negative states */
  for (i = 0; i < numstates; i++) {
    if (polarity[i] == -1) {
      ks_eval_readwave_negatestate(readwave, i);
    }
  }

  free(precrushers);
  free(postcrushers);

  return SUCCESS;
}




void ks_eval_readwave_negatestate(KS_READWAVE* readwave, int state) {
  if (state == 0) {
    KS_WAVE* wave = &readwave->grad;
    wave->max_amp *= -1;
    wave->min_amp *= -1;
    float min_amp = wave->max_amp;
    wave->max_amp = wave->min_amp;
    wave->min_amp = min_amp;
    wave->area *= -1;
  }

  ks_waveform_multiplyval(readwave->grad_states[state],  -1, readwave->grad.res);
  ks_waveform_multiplyval(readwave->omega_states[state], -1, readwave->omega.res);
}




STATUS ks_eval_readwave_multistated(KS_READWAVE* readwave, KS_WAVE* acq_waves, const int numstates, float pre_crusher_area, float post_crusher_area, int flag_symmetric_padding, ks_enum_crusher_strategy pre_strategy, ks_enum_crusher_strategy post_strategy) {
  /* TODO: Remove ? */
  ks_crusher_constraints pre_crusher_constraints = KS_INIT_CRUSHER_CONSTRAINT;
  pre_crusher_constraints.ampmax = ks_syslimits_ampmax(loggrd);
  pre_crusher_constraints.slew = ks_syslimits_slewrate(loggrd);
  pre_crusher_constraints.min_duration = 0;
  pre_crusher_constraints.area = pre_crusher_area;
  pre_crusher_constraints.strategy = pre_strategy;
  ks_crusher_constraints post_crusher_constraints = pre_crusher_constraints;
  post_crusher_constraints.area = post_crusher_area;
  post_crusher_constraints.strategy = post_strategy;

  return ks_eval_readwave_constrained(readwave, acq_waves, numstates,
                                      flag_symmetric_padding,
                                      pre_crusher_constraints,
                                      post_crusher_constraints);
}




STATUS ks_eval_readwave(KS_READWAVE* readwave, KS_WAVE* acq_wave, float pre_crusher_area, float post_crusher_area, int flag_symmetric_padding, ks_enum_crusher_strategy pre_strategy, ks_enum_crusher_strategy post_strategy) {

  ks_crusher_constraints pre_crusher_constraints = KS_INIT_CRUSHER_CONSTRAINT;
  pre_crusher_constraints.ampmax = ks_syslimits_ampmax(loggrd);
  pre_crusher_constraints.slew = ks_syslimits_slewrate(loggrd);
  pre_crusher_constraints.min_duration = 0;
  pre_crusher_constraints.area = pre_crusher_area;
  pre_crusher_constraints.strategy = pre_strategy;
  ks_crusher_constraints post_crusher_constraints = pre_crusher_constraints;
  post_crusher_constraints.area = post_crusher_area;
  post_crusher_constraints.strategy = post_strategy;

  return ks_eval_readwave_constrained(readwave, acq_wave, 1,
                                      flag_symmetric_padding,
                                      pre_crusher_constraints,
                                      post_crusher_constraints);
}




#ifndef DOXYGEN_EXCLUDE


STATUS ks_eval_readtrap1p(KS_READTRAP *readtrap, const char * const desc) {

  return ks_eval_readtrap_constrained(readtrap, desc,
                                      ks_syslimits_ampmax1p(loggrd), ks_syslimits_slewrate1p(loggrd));

}




/*-*/

#endif /* DOXYGEN_EXCLUDE */




void ks_eval_phaseviewtable(KS_PHASER *phaser) {
  int i, index, firstline, lastline, halfacslines;
  short viewonflag[KS_MAX_PHASEDYN];

  /* reset arrays */
  for (i = 0; i < KS_MAX_PHASEDYN; i++) {
    phaser->linetoacq[i] = 0;
    viewonflag[i] = 0;
  }

  if (phaser->nover == 0) {
    firstline = 0;
    lastline = phaser->res - 1;
  } else if (phaser->nover > 0) { /* fractional ky upper half */
    firstline = 0;
    lastline = phaser->res / 2 + phaser->nover - 1;
  } else {
    firstline = phaser->res / 2 + phaser->nover; /* NOTE: phase->nover here negative ! */
    lastline = phaser->res - 1;
  }


  int acscenter = (phaser->res / 2) + phaser->acsshift;

  /* lower half of k-space: mark the lines to acquire with '1' */
  halfacslines = phaser->nacslines / 2;
  for (i = acscenter; i <= lastline; i++) {
    if (((i + 1) % phaser->R) == 1 || phaser->R == 1) {
      viewonflag[i] = 1;
    } else if (halfacslines > 0) {
      viewonflag[i] = 1;
      halfacslines--;
    }
  }

  /* upper half of k-space: mark the lines to acquire with '1' */
  halfacslines = phaser->nacslines - (phaser->nacslines / 2) + halfacslines;
  for (i = acscenter - 1; i >= 0; i--) {
    if (((i + 1) % phaser->R) == 1 || phaser->R == 1) {
      viewonflag[i] = 1;
    } else if (halfacslines > 0) {
      viewonflag[i] = 1;
      halfacslines--;
    }
  }

  /* sweep kspace linearly and save the line numbers to acquire.
     firstline/lastline handles full/partial Fourier */
  index = 0;
  for (i = firstline; i <= lastline; i++) {
    if (viewonflag[i]) {
      phaser->linetoacq[index++] = i;
    }
  }
  phaser->numlinestoacq = index;

}




STATUS ks_eval_adjust_res_and_nover(int* result_nover, int* result_res, const int desired_nover, const int desired_res, const int R) {
  int abs_nover = abs(desired_nover);

  /* Full Fourier */
  if (abs_nover == 0) {
    *result_res = RUP_FACTOR(desired_res, 2);
    return SUCCESS;
  }

  /* Partial Fourer */
  const int sign_nover = (desired_nover >= 0) ? 1 : -1;
  if (R % 2) { /* R is odd */
    *result_res = RUP_FACTOR(desired_res - 2*R, 4*R);
    *result_nover = RUP_FACTOR(desired_nover - R, 2*R);
  } else { /* R is even */
    *result_res = RUP_FACTOR(desired_res - R, 2*R);
    *result_nover = RUP_FACTOR(desired_nover - R/2, R);
  }
  *result_nover *= sign_nover;

  return SUCCESS;  
} /* ks_eval_adjust_res_and_nover */


STATUS ks_eval_partial_fourier(int *nover, KS_PF_EARLYLATE *earlylate_te, int* res, const float pf_factor, const int R, const int min_nover) { 

  if (fabs(pf_factor) > 1.0) {
  return KS_THROW("%s: TEmodifer (3rd arg) must be in range [-1.0,1.0]", __FUNCTION__);
  }
  if (*res < 8 || *res > 1024) {
  return KS_THROW("res (3rd arg) must be in range [8, 1024]");
  }
  if (nover == NULL || earlylate_te == NULL) {
  return KS_THROW("Outputs point to NULL");
  }

  if (areSame(pf_factor,0.0)) { /* full Fourier */
  *nover = 0;
  *earlylate_te = KS_PF_NO;
  } else {
  float min = (float) min_nover;
  float max = *res/2.0 - min_nover;

  *nover = RUP_FACTOR((int) ((max-min) * (1.0 - fabs(pf_factor)) + min), 2);
  if (*nover == 0) {
  *earlylate_te = KS_PF_NO;
  }
  *earlylate_te = (pf_factor < 0) ? KS_PF_EARLY : KS_PF_LATE;
  }

  STATUS s = ks_eval_adjust_res_and_nover(nover, res, *nover, *res, R);
  KS_RAISE(s);

  return s;

} /* ks_eval_partial_fourier() */


STATUS ks_eval_phaser_adjustres(KS_PHASER *phaser, const char * const desc) {

  int kspacelines_noacc;

  if (phaser->nover == 0)
    kspacelines_noacc = phaser->res;
  else
    kspacelines_noacc = phaser->res / 2 + abs(phaser->nover);

  if (kspacelines_noacc < phaser->R) {
    /* Number of k-space lines less than R */
    return KS_THROW("(%s): R cannot exceed %d", desc, kspacelines_noacc);
  } else if (kspacelines_noacc == phaser->R) {
  /* Number of k-space lines equal to R. Used for EPI when R is used as shots to produce ETL = 1.
    Just make sure that res is even */
  return SUCCESS;
  }

  if (phaser->nover != 0) {
    /* partial Fourier */
    const int sign_nover = (phaser->nover >= 0) ? 1 : -1;
    phaser->nover = abs(phaser->nover);
    int invalidnover = TRUE;

    /* round res and nover to nearest valid values */
    if (phaser->R % 2) { /* R is odd */
      phaser->res   = RUP_FACTOR(phaser->res   - 2 * phaser->R, 4 * phaser->R);
      phaser->nover = RUP_FACTOR(phaser->nover - phaser->R, 2 * phaser->R);
      invalidnover = phaser->nover < (2 * phaser->R) || phaser->nover > (phaser->res / 2);
    } else { /* R is even */
      phaser->res   = RUP_FACTOR(phaser->res   - phaser->R, 2 * phaser->R);
      phaser->nover = RUP_FACTOR(phaser->nover - phaser->R/2, phaser->R);
      invalidnover = phaser->nover < phaser->R || phaser->nover > (phaser->res / 2);
    }
    phaser->nover *= sign_nover;

    if (invalidnover) {
      return KS_THROW("(%s): Valid #overscans not found with res=%d and nover=%d", desc, phaser->res, phaser->nover);
    }

  } else {
    /* full Fourier */
    if (phaser->shotresalign == TRUE) {
      int tmp_res = 0;

      tmp_res = RUP_FACTOR( phaser->res - phaser->R/2, phaser->R);

      if (tmp_res % 2 > 0) {
        phaser->res = RUP_FACTOR(phaser->res - phaser->R, 2 * phaser->R);
      } else {
        phaser->res = tmp_res;
      }
    } else {
      phaser->res = RUP_FACTOR(phaser->res, 2);
    }

  }

  return SUCCESS;
}




STATUS ks_eval_phaser_setaccel(KS_PHASER *phaser, int min_acslines, float R) {

  if (R <= 1.0) {
    phaser->R = 1.0;
    phaser->nacslines = 0;
    return SUCCESS;
  }

  /* Get integer acceleration and store the Rfraction for ARC mode */
  double Rfraction = ceil(R) - R;
  if (Rfraction > 0.9999) { /* handle roundoff issues for e.g. R=2.0000001 */
    Rfraction = 0.0;
  }
  phaser->R = floor(R + Rfraction + 0.5); /* round-up R to nearest integer */

  if (min_acslines == 0) {
    /* ASSET mode */

    /* phaser.nacslines = 0 triggers ASSET scan mode in GEReq_predownload_setrecon_accel() */
    phaser->nacslines = 0;

  } else {
    /* ARC mode */

    double totlines_spanned = (phaser->nover != 0) ? (phaser->res / 2 + abs(phaser->nover)) : phaser->res;
    double acqlines_R       = totlines_spanned / phaser->R;
    double acqlines_floorR  = totlines_spanned / FMax(2, 1.0, floor(R));

    /* adapt #acs lines based on the fraction of the acceleration factor, but never go below min_acslines */
    phaser->nacslines = IMax(2, (int) ((acqlines_floorR - acqlines_R) * Rfraction), min_acslines);

  }

  return SUCCESS;
}




/*-*/

STATUS ks_eval_phaser_constrained(KS_PHASER *phaser, const char * const phasername,
                                  float ampmax, float slewrate, int minduration) {

  STATUS status;
  float phasepixelarea;

  ks_init_trap(&phaser->grad);

  if (phaser->fov < 0.1 || phaser->fov > 2000.0) {
    return KS_THROW("(%s): field 'fov' (%g) must be in the range 0.1-2000 mm", phasername, phaser->fov);
  }

  /* adjust res and possibly nover to allow both ARC and ASSET scans with full and partial ky Fourier */
  status = ks_eval_phaser_adjustres(phaser, phasername);
  KS_RAISE(status);

  phasepixelarea = ks_calc_fov2gradareapixel(phaser->fov); /* [(G/cm)*usec] */

  /* area required to reach the outermost phase encoding step (incl. areaoffset)
  N.B.: The *sign* of the area is controlled in scan using ks_scan_phaser_toline(), hence on HOST all phase encoding gradient will be positive, unlike TGT */
  phaser->grad.area = ((phaser->res - 1.0) / 2.0) * phasepixelarea + fabs(phaser->areaoffset); /* .areaoffset is used in 3D imaging to embed e.g. a slice sel. rephaser in the phase encoding gradient */

  /* setup phaser trap */
  if (!areSame(phaser->grad.area, 0.0)) {
    status = ks_eval_trap_constrained(&phaser->grad, phasername, ampmax, slewrate, minduration);
    KS_RAISE(status);
  }

  /* setup phaser->linetoacq[] */
  ks_eval_phaseviewtable(phaser);


  return SUCCESS;

}




/*-*/

STATUS ks_eval_phaser(KS_PHASER *phaser, const char * const phasername) {

  return ks_eval_phaser_constrained(phaser, phasername, ks_syslimits_ampmax(loggrd), ks_syslimits_slewrate(loggrd), 0);

}




STATUS ks_eval_phaser2(KS_PHASER *phaser, const char * const phasername) {

  return ks_eval_phaser_constrained(phaser, phasername, ks_syslimits_ampmax2(loggrd), ks_syslimits_slewrate2(loggrd), 0);

}




STATUS ks_eval_phaser1(KS_PHASER *phaser, const char * const phasername) {

  return ks_eval_phaser_constrained(phaser, phasername, ks_syslimits_ampmax1(loggrd), ks_syslimits_slewrate1(loggrd), 0);

}




STATUS ks_eval_phaser1p(KS_PHASER *phaser, const char * const phasername) {

  return ks_eval_phaser_constrained(phaser, phasername, ks_syslimits_ampmax1p(loggrd), ks_syslimits_slewrate1p(loggrd), 0);

}




STATUS ks_eval_wave(KS_WAVE *wave, const char * const desc, const int res, const int duration, const KS_WAVEFORM waveform) {
  KS_DESCRIPTION tmpdesc;

  strncpy(tmpdesc, desc, KS_DESCRIPTION_LENGTH / 2);
  /* don't allow NULL or a description with leading space */
  if (desc == NULL || desc[0] == ' ') {
    return KS_THROW("wave name (2nd arg) cannot be NULL or leading space");
  }

  if (duration < 0 || res < 0) {
    return KS_THROW("%s: 'res' and 'duration' must be positive", desc);
  }
  if (res > KS_MAXWAVELEN) {
    return KS_THROW("%s: 'res' cannot exceed %d", wave->description, KS_MAXWAVELEN);
  }
  if (duration > 0 && (duration % res)) {
    return KS_THROW("%s: 'duration' (%d) [us] must be divisible by 'res' (%d)", desc, duration, res);
  }
  if (duration > 0 && (duration / res) < 2) {
    return KS_THROW("%s: 'duration' (%d) [us] must be at least 2x 'res' (%d)", desc, duration, res);
  }
  if ((waveform == wave->waveform) || (waveform == NULL) ) {
    return KS_THROW("%s: cannot eval a wave with its own or NULL waveform", desc);
  }
  if (wave->waveform == NULL) {
    return KS_THROW("%s: waveform is NULL, did you forget to allocate memory?", desc);
  }

  /* reset wave object */
  ks_init_wave(wave);

  /* copy the waveform */
  memcpy(wave->waveform, waveform, sizeof(float) * res);

  /* fill in wave object */
  strcpy(wave->description, tmpdesc);
  wave->res = res;
  wave->duration = duration;

  ks_wave_compute_params(wave);

  return SUCCESS;

}




STATUS ks_eval_wave_file(KS_WAVE *wave, const char * const desc, int res, int duration, const char * const filename, const char *const format) {
  KS_DESCRIPTION tmpdesc;
  int i;

  strncpy(tmpdesc, desc, KS_DESCRIPTION_LENGTH / 2);
  /* don't allow empty description or a description with leading space */
  if (desc == NULL || desc[0] == ' ') {
    return ks_error("ks_eval_wave_file: wave name (2nd arg) cannot be NULL or leading space");
  }
  if (res % 2) {
    return ks_error("ks_eval_wave_file(%s): 'res' must be even", wave->description);
  }
  if (duration > 0 && (duration % res)) {
    return ks_error("ks_eval_wave_file(%s): 'duration' (%d) [us] must be divisible by 'res' (%d)", desc, duration, res);
  }
  if (duration > 0 && (duration / res) < 2) {
    return ks_error("ks_eval_wave_file(%s): 'duration' (%d) [us] must be at least 2x 'res' (%d)", desc, duration, res);
  }
  if (duration < 0 || duration > 4 * KS_MAXWAVELEN) {
    return ks_error("ks_eval_wave_file(%s): 'duration' [us] must be in the range [0, %d]", desc, 4 * KS_MAXWAVELEN);
  }

  /* reset wave object */
  ks_init_wave(wave);

  /* fill in wave object */
  strcpy(wave->description, tmpdesc);
  wave->res = res;
  wave->duration = duration;

  /* read from file */
  if (!strncasecmp(format, "ge", 2)) { /* short format with 32-byte header and big-endian short int */
    KS_IWAVE iwave;
    uextwave(iwave, res, (char *) filename);
    for (i = 0; i < res; i++)
      wave->waveform[i] = (float) iwave[i] / (float) MAX_PG_WAMP;
  } else if (!strncasecmp(format, "short", 5)) {
    KS_IWAVE iwave;
    FILE *fp;
    int n;
    if ((fp = fopen(filename, "r")) == NULL)
      return ks_error("ks_eval_wave_file (%s): Error opening file '%s'", wave->description, filename);
    if ((n = fread(iwave, sizeof(short), res, fp)) < res)
      return ks_error("ks_eval_wave_file (%s): Only %d out of %d elements read from file '%s'", wave->description, n, res, filename);
    fclose(fp);
    for (i = 0; i < res; i++)
      wave->waveform[i] = (float) iwave[i] / (float) MAX_PG_WAMP;
  } else if (!strncasecmp(format, "float", 5)) {
    FILE *fp;
    int n;
    if ((fp = fopen(filename, "r")) == NULL)
      return ks_error("ks_eval_wave_file (%s): Error opening file '%s'", wave->description, filename);
    if ((n = fread(wave->waveform, sizeof(float), res, fp)) < res)
      return ks_error("ks_eval_wave_file (%s): Only %d out of %d elements read from file '%s'", wave->description, n, res, filename);
    fclose(fp);
  } else {
    return ks_error("ks_eval_wave_file (%s): 'format' (5th arg) must be 'ge','short' or 'float'", wave->description);
  }


  return SUCCESS;

} /* ks_eval_wave_file */



STATUS ks_eval_mirrorwave(KS_WAVE *wave) {
  KS_WAVEFORM tmpwave;
  int i;

  memcpy(tmpwave, wave->waveform, sizeof(float) * wave->res);

  for (i = 0; i < wave->res; i++) {
    wave->waveform[i] = tmpwave[wave->res-1 - i];
  }
  return SUCCESS;
}




STATUS ks_eval_mirrorwaveform(KS_WAVEFORM waveform, int res) {
  KS_WAVEFORM tmpwave;
  int i;

  memcpy(tmpwave, waveform, sizeof(float) * res);

  for (i = 0; i < res; i++) {
    waveform[i] = tmpwave[res - 1 - i];
  }
  return SUCCESS;
}




/*******************************************************************************************************
 *  RF object (KS_RF)
 *******************************************************************************************************/



STATUS ks_eval_rf_sinc(KS_RF *rf, const char * const desc, double bw, double tbw, float flip, int wintype) {
  KS_DESCRIPTION tmpdesc;
  int i;
  double x, window, rfenvelope;
  int duration = RUP_GRD(tbw / bw * 1e6); /* [us]. We round up to nearest multiple of 4 (GRAD_UPDATE_TIME) to better align to gradients */
  int res = duration / RF_UPDATE_TIME; /* # sample points. RF_UPDATE_TIME = 2 => res always even */

  /* don't allow empty description or a description with leading space */
  if (desc == NULL || desc[0] == ' ') {
    return KS_THROW("RF name (2nd arg) cannot be NULL or leading space");
  }
  strncpy(tmpdesc, desc, KS_DESCRIPTION_LENGTH/2);
  tmpdesc[KS_DESCRIPTION_LENGTH/2] = 0;

  /* reset RF object */
  ks_init_rf(rf);

  /* fill in wave object */
  ks_create_suffixed_description(rf->rfwave.description, tmpdesc, ".wave");

  if (bw < 2 || bw > 100000) {
    return KS_THROW("%s: 'bw' (%g, 3rd arg) must be in range [2,100000] Hz", rf->rfwave.description, bw);
  }
  if (tbw < 2 || tbw > 100) {
    return KS_THROW("%s: 'tbw' (%g, 4th arg) must be in range [2,100]", rf->rfwave.description, tbw);
  }
  if (res > KS_MAXWAVELEN || res < 4) {
    return KS_THROW("%s: combination of 'bw' and 'tbw' resulted in a resolution outside the valid range [4, %d]", rf->rfwave.description, KS_MAXWAVELEN);
  }
  if (flip <= 0 || flip > 10000.0) {
    return KS_THROW("%s: 'flip' must be in range (0, 10000]", rf->rfwave.description);
  }

  rf->rfwave.res = res;
  rf->rfwave.duration = duration;
  rf->iso2end = duration / 2;
  rf->start2iso = duration / 2;
  rf->bw = bw;
  rf->flip = flip;

  /* generate waveform with max 1.0 */
  for (i = 0 ; i < (res - 1) ; i++) {
    x = 2.0 * i / (res - 1) - 1.0;

    if (fabs(2.0 * PI * (tbw / 4.0)*x) > 1e-6) {
      rfenvelope = sin(2.0 * PI * (tbw / 4.0) * x) / (2.0 * PI * (tbw / 4.0) * x);
    } else {
      rfenvelope = 1.0;
    }

    window = 1.0; /* init to no filter */

    if (wintype == KS_RF_SINCWIN_HAMMING) {
      window = 0.54 + 0.46 * cos(PI * x);
    } else if (wintype == KS_RF_SINCWIN_HANNING) {
      window = (1.0 + cos(PI * x)) * 0.5;
    } else if (wintype == KS_RF_SINCWIN_BLACKMAN) {
      window = 0.42 + 0.5 * cos(PI * x) + 0.08 * cos(2 * PI * x);
    } else if (wintype == KS_RF_SINCWIN_BARTLETT) {
      if (x < 0.0)
        window = x + 1.0;
      else
        window = 1.0 - x;
    }

    rf->rfwave.waveform[i] = rfenvelope * window;

  } /* for */

  /* initialize amplitude to 1 */
  rf->amp = 1;

  /* set the usage of the RF PULSE to off until .base.ninst > 0 (see ks_pg_rf() on HOST) */
  rf->rfpulse.activity = PSD_PULSE_OFF;

  /* initialize number of occurrences of RF pulse to zero */
  rf->rfpulse.num = 0;

  /* setup rfpulse structure */
  return ks_eval_rfstat(rf);
}




STATUS ks_eval_rf_secant(KS_RF *rf, const char * const desc, float A0, float tbw, float bw) {
  STATUS status;
  int debug = 1;

  /* reset RF object */
  ks_init_rf(rf);

  KS_DESCRIPTION tmpdesc;
  int i;

  /* don't allow empty description or a description with leading space */
  if (desc == NULL || desc[0] == ' ') {
    return ks_error("ks_eval_rf_secant: 'RF name' (2nd arg) cannot be NULL or leading space");
  }
  strncpy(tmpdesc, desc, KS_DESCRIPTION_LENGTH/2);
  tmpdesc[KS_DESCRIPTION_LENGTH/2] = 0;

  /* fill in wave object */
  ks_create_suffixed_description(rf->rfwave.description, tmpdesc, ".wave");

  if (A0 < 0 || 0.25 < A0) {
    return KS_THROW("%s: 'A0' (%g, 3rd arg) must be in range [0,0.25] Gauss", rf->rfwave.description, A0);
  }
  if (tbw < 2 || 20 < tbw) {
    return KS_THROW("%s: 'tbw' (%g, 4th arg) must be in range [2,20]", rf->rfwave.description, tbw);
  }
  if (bw < 100 || 10000 < bw) {
    return KS_THROW("%s: 'bw' (%g, 5th arg) must be in range [0,10000] Hz", rf->rfwave.description, bw);
  }

  int duration = RUP_GRD((int)(1000000.0 * tbw / bw));
  int res = duration / GRAD_UPDATE_TIME; /* # sample points. RF_UPDATE_TIME = 2 => res always even */

  if (res < 4 || KS_MAXWAVELEN < res) {
    return KS_THROW("%s: tbw / bw resulted in a resolution(%d)/duration(%d) outside the valid range [4, %d]", rf->rfwave.description, res, duration, KS_MAXWAVELEN);
  }

  rf->rfwave.res = res;
  rf->rfwave.duration = duration;
  rf->iso2end = duration / 2;
  rf->start2iso = duration / 2;

  /* calculate pulse properties */
  float y = 5; /* 5 is a standard value from literature */
  float B = (bw * PI) / y;
  float Acheck = (PI * bw) / (2.0 * PI * 4258 * sqrt(y)); /* Gauss */

  if (Acheck > A0 * 0.5) {
    return KS_THROW("%s: The adiabatic condition might not be met since %g !>> %g (needs to be at least half)", rf->rfwave.description, A0, Acheck);
  }

  rf->bw = bw;
  rf->flip = 180.0;

  double rfenvelope, phaseenvelope, tau, sech;
  for (i = 0 ; i < res; i++) {

    /* range from -2pi to 2pi */
    tau = ((i / (((float)res - 1.0) * 0.5)) - 1.0) * 2.0 * PI;

    sech = 1.0 / cosh(tau * B * 0.001);
    rfenvelope = A0 * sech;
    phaseenvelope = y * log(sech) + y * log(A0);

    rf->rfwave.waveform[i] = rfenvelope;
    rf->thetawave.waveform[i] = phaseenvelope * (180.0/PI);

  }

  if (debug == 1) {
    ks_print_waveform(rf->rfwave.waveform, "secant_a.txt", rf->rfwave.res);
    ks_print_waveform(rf->thetawave.waveform, "secant_p.txt", rf->rfwave.res);
    ks_error("HS: A0=%f >> Ac=%f, bw=%f, tbw=%f", A0, Acheck, bw, tbw);
    ks_error("HS: y=%f, B=%f", y, B);
  }

  /* Normalize waveform to one & initialize amplitude to 1 */
  ks_wave_multiplyval(&rf->rfwave, 1.0 / ks_wave_absmax(&rf->rfwave));
  rf->amp = 1;

  /* set the usage of the RF PULSE to off until .base.ninst > 0 (see ks_pg_rf() on HOST) */
  rf->rfpulse.activity = PSD_PULSE_OFF;

  /* initialize number of occurrences of RF pulse to zero */
  rf->rfpulse.num = 0;

  /* setup rfpulse structure */
  status = ks_eval_rfstat(rf);
  KS_RAISE(status);

  rf->rfpulse.max_b1 = A0;
  rf->thetawave.res = res;
  rf->thetawave.duration = duration;
  rf->role = KS_RF_ROLE_INV;
  return status;
}




STATUS ks_eval_rf_hard(KS_RF *rf, const char * const desc, int duration, float flip) {
  KS_DESCRIPTION tmpdesc;
  int i;

  /* don't allow empty description or a description with leading space */
  if (desc == NULL || desc[0] == ' ') {
    return KS_THROW("RF name (2nd arg) cannot be NULL or leading space");
  }
  strncpy(tmpdesc, desc, KS_DESCRIPTION_LENGTH/2);
  tmpdesc[KS_DESCRIPTION_LENGTH/2] = 0;

  /* reset RF object */
  ks_init_rf(rf);

  /* fill in wave object */
  ks_create_suffixed_description(rf->rfwave.description, tmpdesc, ".wave");

  if (duration > KS_MAXWAVELEN * RF_UPDATE_TIME || duration < 4) {
    return KS_THROW("%s: 'duration' (3rd arg) must be in range [4, %d]", rf->rfwave.description, KS_MAXWAVELEN * RF_UPDATE_TIME);
  }
  if (duration % 2) {
    return KS_THROW("%s: 'duration' (3rd arg) must be divisible by 2", rf->rfwave.description);
  }

  rf->rfwave.res = duration / RF_UPDATE_TIME;
  rf->rfwave.duration = duration;
  rf->iso2end = duration / 2;
  rf->start2iso = duration / 2;
  rf->bw = 0;
  rf->flip = flip;

  /* generate waveform with max 1.0 */
  for (i = 0 ; i < rf->rfwave.res ; i++) {
    rf->rfwave.waveform[i] = 1.0;
  }

  /* initialize amplitude to 1 */
  rf->amp = 1;

  /* set the usage of the RF PULSE to off until .base.ninst > 0 (see ks_pg_rf() on HOST) */
  rf->rfpulse.activity = PSD_PULSE_OFF;

  /* initialize number of occurrences of RF pulse to zero */
  rf->rfpulse.num = 0;

  /* setup rfpulse structure */
  STATUS status = ks_eval_rfstat(rf);
  KS_RAISE(status);

  return SUCCESS;
}




STATUS ks_eval_rf_hard_optimal_duration(KS_RF *rf, const char * const desc, int order, float flip, float offsetFreq) {

  if (order < 1) {
    return KS_THROW("%s: 'order' (%d, 3rd arg) must be positive", desc, order);
  }

  int duration = RUP_FACTOR((int)(1000000.0 * sqrt(pow(2.0 * PI * (float)order, 2) - pow((float)flip * (PI / 180.0), 2)) / fabs(2.0 * PI * offsetFreq)), GRAD_UPDATE_TIME);

  return ks_eval_rf_hard(rf, desc, duration, flip);

}




STATUS ks_eval_rf_binomial(KS_RF *rf, const char * const desc, int offResExc, int nPulses, float flip, float offsetFreq, int MTreduction) {
  KS_DESCRIPTION tmpdesc;
  int i, j;

  /* Don't allow empty description or a description with leading space */
  if (desc == NULL || desc[0] == ' ') {
    return KS_THROW("RF name (2nd arg) cannot be NULL or leading space");
  }
  strncpy(tmpdesc, desc, KS_DESCRIPTION_LENGTH/2);
  tmpdesc[KS_DESCRIPTION_LENGTH/2] = 0;

  /* Reset RF object */
  ks_init_rf(rf);

  /* Fill in wave object */
  ks_create_suffixed_description(rf->rfwave.description, tmpdesc, ".wave");
  sprintf(rf->designinfo, "%s", tmpdesc);

  if (offResExc < 0 || offResExc > 1) {
    return KS_THROW("%s: 'offResExc' (3rd arg) must be either 0 or 1", rf->rfwave.description);
  }
  if (nPulses < 2 || nPulses > 10) {
    return KS_THROW("%s: 'nPulses' (4th arg) must be in range [2, 10]", rf->rfwave.description);
  }

  int pascalstriangle[9][10] = { {1,1},
                                {1,2,1},
                               {1,3,3,1},
                              {1,4,6,4,1},
                            {1,5,10,10,5,1},
                           {1,6,15,20,15,6,1},
                          {1,7,21,35,35,21,7,1},
                         {1,8,28,56,70,56,28,8,1},
                       {1,9,36,84,126,126,84,36,9,1} };

  int P[10];
  int pulseSum = 0;
  for (i = 0; i < nPulses; i++) {
    P[i] = pascalstriangle[nPulses - 2][i];
    pulseSum += P[i];
  }

  /* Calculate subpulse separation */
  float tau = fabs(1.0 / (2.0 * offsetFreq)); /* sec */

  /* Calculate number of samples so that each subpulse is as close as possible to maxB1, without exceeding it */
  float maxB1 = 0.20; /* Gauss */
  float pulseArea = (float) flip * (PI / 180.0); /* radians or Hz (depending on how you look at it) */
  float dt = RF_UPDATE_TIME / 1000000.0; /* sec */
  float subArea[10];
  int ntps[10];
  for (i = 0; i < nPulses; i++) {
    subArea[i] = (P[i] * pulseArea) / (pulseSum * 2.0 * PI * GAM); /* Gauss * sec */
    if (MTreduction) {
      ntps[i] = ceil(tau / dt); 
    } else {
      ntps[i] = ceil(subArea[i] / (maxB1 * dt)); /* number of time points in sub pulse */
      if (ntps[i] < 32 ) {
        ntps[i] = 32;
      }
    }
  }

  int ntp = 0;
  for (i = 0; i < nPulses; i++) {
    if (ntps[i]>ntp) {
      ntp = ntps[i];
    }
  }

  int idx = 0;
  for (i = 0; i < nPulses; i++) {

    /* Calculate how much we have to attenuate this pulse
       to account for the fact that with this dwell time
       we couldn't hit the target flip exactly */
    float currentSubArea = ntp * dt * maxB1; /* Gauss * sec */
    float amp = maxB1 * (subArea[i] / currentSubArea); /* Gauss */

    /* Flip every other sub pulse for off resonace excitaion */
    if (offResExc) {
      /* make sure the second center sub-pulse flips in the right direction */
      amp *= pow(-1.0, (float)(i + nPulses/2 % 2));
    }

    /* Stitch on the sub pulse */
    for (j = 0; j < ntp; j++) {
      rf->rfwave.waveform[idx] = amp;
      idx++;
    }

    /* Stitch on the gap, skip for last sub pulse */
    if (i < nPulses - 1) {
      int ntp_gap = ceil((tau/dt) - (float)ntp);
      if (MTreduction) {
        ntp_gap = 0;
      } 
      for (j = 0; j < ntp_gap; j++) {
        rf->rfwave.waveform[idx] = 0.0;
        idx++;
      }
    }
  }

  /* Make sure the resolution is divisible by 2 and the duration divisible by 4 */
  rf->rfwave.res = RUP_FACTOR(idx, 4);
  rf->rfwave.duration = rf->rfwave.res * RF_UPDATE_TIME;
  rf->iso2end = rf->rfwave.duration / 2;
  rf->start2iso = rf->rfwave.duration / 2;
  rf->bw = 0; /* ? */
  rf->flip = flip;

  /* If the resolution has been rounded up, place zeros at the end */
  for (j = idx; j < rf->rfwave.res; j++) {
    rf->rfwave.waveform[j] = 0.0;
  }

  /* Normalize waveform to one */
  ks_wave_multiplyval(&rf->rfwave, 1.0 / ks_wave_absmax(&rf->rfwave));

  /* Initialize amplitude to one */
  rf->amp = 1;

  /* Set the usage of the RF PULSE to off until .base.ninst > 0 (see ks_pg_rf() on HOST) */
  rf->rfpulse.activity = PSD_PULSE_OFF;

  /* Never allow binomial RF pulses to be stretched. We can avoid this by always setting extgradfile = 1 (even if this is not the case) */
  rf->rfpulse.extgradfile = 1;

  /* Initialize number of occurrences of RF pulse to zero */
  rf->rfpulse.num = 0;

  /* Save original waveform to fool rfstat, with abs(waveform) */
  float *saveWave = (float*)alloca(rf->rfwave.res * sizeof(float));
  if (offResExc) {
    memcpy(saveWave, rf->rfwave.waveform, rf->rfwave.res * sizeof(float));
    for (j = 0; j < rf->rfwave.res; j++) {
      rf->rfwave.waveform[j] = fabs(rf->rfwave.waveform[j]);
    }
  }

  /* Setup rfpulse structure */
  STATUS status = ks_eval_rfstat(rf);
  KS_RAISE(status);

  /* Put back the correct waveform */
  if (offResExc) {
    memcpy(rf->rfwave.waveform, saveWave, rf->rfwave.res * sizeof(float));
  }

  return SUCCESS;

}




STATUS ks_eval_mt_rf_binomial(KS_RF *rf, const char * const desc, int sub_pulse_dur, int nPulses, int prp) {
  KS_DESCRIPTION tmpdesc;
  int i, j, k;

  if (prp < sub_pulse_dur) {
    return ks_error("ks_eval_mt_rf_binomial: PRP(=%d) must be bigger than sub_pulse_dur(=%d)", prp, sub_pulse_dur);
  }

  double offsetFreq = 421.5420;
  if (cffield != 30000) {
    offsetFreq *= 0.5;
  }

  /* constraining the pulse repetition period to be greater or less than 1/(2 x chemical shift). */
  double nogo = 1.0/(2.0*offsetFreq)*1000000.0;
  if (areSameRelative(prp, nogo, 0.01)) {
    return ks_error("ks_eval_mt_rf_binomial: PRP(=%d) is too similar to %f (would cause artifacts)", prp, nogo);
  }

  /* Don't allow empty description or a description with leading space */
  if (desc == NULL || desc[0] == ' ') {
    return ks_error("ks_eval_mt_rf_binomial: RF name (2nd arg) cannot be NULL or leading space");
  }
  strncpy(tmpdesc, desc, KS_DESCRIPTION_LENGTH/2);
  tmpdesc[KS_DESCRIPTION_LENGTH/2] = 0;

  /* Reset RF object */
  ks_init_rf(rf);

  /* Fill in wave object */
  sprintf(rf->rfwave.description, "%s.wave", tmpdesc);
  sprintf(rf->designinfo, "%s", tmpdesc);

  double dt = 2e-6; /* sec */
  float pulseArea = (float) 90.0 * 4.0 * (PI / 180.0); /* radians or Hz (depending on how you look at it) */
  double subArea = (pulseArea) / (4.0 * 2.0 * PI * GAM); /* Gauss * sec */
  int ntps = ceil(sub_pulse_dur/4/RF_UPDATE_TIME); /* number of time points in sub pulse */
  float maxB1 = subArea/(ntps * dt); /* Gauss */

  /* Calculate how much we have to attenuate this pulse
       to account for the fact that with this dwell time
       we couldn't hit the target flip exactly */
  double currentSubArea = ntps * dt * maxB1; /* Gauss * sec */
  float amp = maxB1 * (subArea / currentSubArea); /* Gauss */

  int ntp_gap = ceil((prp/RF_UPDATE_TIME) - (float)ntps); 

  int idx = 0, sub_dir = 1, dir = 1;
  for (i = 0; i < nPulses; i++) {

     dir = pow(-1.0, (float)(i + nPulses/2 % 2));

    for (k = 0; k < 4; k++) { 

      if (k == 1 || k == 2) {
        sub_dir = -1;
      } else {
        sub_dir = 1;
      }

      /* Stitch on the sub pulse */
      for (j = 0; j < ntps; j++) {
        rf->rfwave.waveform[idx] = amp * sub_dir * dir;
        idx++;
      }

    }
    if ((nPulses > 1) && (i != nPulses-1)) {
      /* Stitch on the pause */
      for (j = 0; j < ntp_gap; j++) {
        rf->rfwave.waveform[idx] = 0.0;
        idx++;
      }
    }
  }

   /* Make sure the resolution is divisible by 2 and the duration divisible by 4 */
  rf->rfwave.res = RUP_FACTOR(idx, 4);
  rf->rfwave.duration = rf->rfwave.res * RF_UPDATE_TIME;
  rf->iso2end = rf->rfwave.duration / 2;
  rf->start2iso = rf->rfwave.duration / 2;
  rf->bw = 0; /* ? */
  rf->flip = 90 + 180 + 90;

  /* If the resolution has been rounded up, place zeros at the end */
  for (j = idx; j < rf->rfwave.res; j++) {
    rf->rfwave.waveform[j] = 0.0;
  }

  /* Normalize waveform to one */
  ks_wave_multiplyval(&rf->rfwave, 1.0 / ks_wave_absmax(&rf->rfwave));

  /* Initialize amplitude to one */
  rf->amp = 1;

  /* Set the usage of the RF PULSE to off until .base.ninst > 0 (see ks_pg_rf() on HOST) */
  rf->rfpulse.activity = PSD_PULSE_OFF;

  /* Never allow binomial RF pulses to be stretched. We can avoid this by always setting extgradfile = 1 (even if this is not the case) */
  rf->rfpulse.extgradfile = 1;

  /* Initialize number of occurrences of RF pulse to zero */
  rf->rfpulse.num = 0;

  /* Save original waveform to fool rfstat, with abs(waveform) */
  float *saveWave = (float*)alloca(rf->rfwave.res * sizeof(float));
  memcpy(saveWave, rf->rfwave.waveform, rf->rfwave.res * sizeof(float));
  for (j = 0; j < rf->rfwave.res; j++) {
    rf->rfwave.waveform[j] = fabs(rf->rfwave.waveform[j]);
  }

  /* Setup rfpulse structure */
  STATUS status = ks_eval_rfstat(rf);
  if (status != SUCCESS) return status;

  /* Put back the correct waveform */
  memcpy(rf->rfwave.waveform, saveWave, rf->rfwave.res * sizeof(float));

  return SUCCESS;
}




STATUS ks_eval_rfstat(KS_RF *rf) {
  double standard_pw = 1e-3;

  double nRFenvelope;
  int indx;

  double area          = 0.0;
  double abswidth      = 0.0;
  double effwidth      = 0.0;
  double dtycyc        = 0.0;
  double max_pw        = 0.0;
  double max_b1        = 0.0;
  double max_int_b1_sq = 0.0;
  double max_rms_b1    = 0.0;


  if (rf->thetawave.res > 0 || rf->omegawave.res > 0) {
    ks_error("ks_eval_rfstat(%s): WARNING: function is not validated for phase/frequency modulated RF-pulses", rf->rfwave.description);
  }
  /* require fields 'duration', 'isodelay' and 'bw' to be set to realistic values */
  if (rf->rfwave.duration < 4 || rf->rfwave.duration > KS_MAXWAVELEN * RF_UPDATE_TIME || rf->rfwave.duration % 2) {
    return ks_error("ks_eval_rfstat: 'rf.rfwave.duration' (%d) must be an even number in the range [4,%d] us", rf->rfwave.duration, KS_MAXWAVELEN * RF_UPDATE_TIME);
  }
  if (rf->bw < 0 || rf->bw > 100000) {
    return ks_error("ks_eval_rfstat: 'rf.bw' (%f) must be in the range [0,100000] Hz", rf->bw);
  }
  if (rf->iso2end < 0 || rf->iso2end > KS_MAXWAVELEN * RF_UPDATE_TIME) {
    return ks_error("ks_eval_rfstat: 'rf.iso2end' (%d) must be in the range [0,%d] us", rf->iso2end, KS_MAXWAVELEN * RF_UPDATE_TIME);
  }
  if (rf->flip < 0.0 || rf->flip > 10000.0) {
    return ks_error("ks_eval_rfstat: 'rf.flip' (%.2f) must be in the range [0,3000] deg", rf->flip);
  }
  /* for reference, see: /ESE_xxxxx/psd/include/private/sar_pm.h  */

  /* normalize RF just in case */
  ks_wave_multiplyval(&rf->rfwave, 1.0 / ks_wave_absmax(&rf->rfwave));


  for (indx = 0; indx < rf->rfwave.res; indx++) {
    nRFenvelope = rf->rfwave.waveform[indx];
    area += nRFenvelope;
    abswidth += fabs(nRFenvelope);
    effwidth += (nRFenvelope * nRFenvelope);

    /* dtycyc: % of Pulse Widths above 5% power */
    if (fabs(nRFenvelope) > 0.05)
      dtycyc++;
  }

  area     /= rf->rfwave.res;
  abswidth /= rf->rfwave.res;
  effwidth /= rf->rfwave.res;
  dtycyc   /= rf->rfwave.res;

  /* max_pw: % of the pulse width whose widest lobe is above 5% power
     GE uses max_pw = dtycyc for most pulses (sar_pm.h)
     Neither max_pw nor dtycyc seem to match well with the values used in GEs product sequences unlike
     the other fields, which seems to agree within a few percent */
  max_pw = dtycyc;

  max_b1 = rf->flip / 360.0 / (area * (rf->rfwave.duration) * 1e-6) / GAM;
  max_int_b1_sq = max_b1 * max_b1 * effwidth * (rf->rfwave.duration) * 1e-6 / standard_pw;
  max_rms_b1 = sqrt(max_int_b1_sq / ((rf->rfwave.duration) * 1e-6) * standard_pw);


  ks_eval_rf_relink(rf);

  rf->rfpulse.abswidth       =  abswidth;
  rf->rfpulse.effwidth       =  effwidth;
  rf->rfpulse.area           =  area;
  rf->rfpulse.dtycyc         =  dtycyc;
  rf->rfpulse.maxpw          =  max_pw;
  rf->rfpulse.num            =  0;
  rf->rfpulse.max_b1         =  max_b1;
  rf->rfpulse.max_int_b1_sq  =  max_int_b1_sq;
  rf->rfpulse.max_rms_b1     =  max_rms_b1;
  rf->rfpulse.nom_fa         =  rf->flip;   /* nom_fa and act_fa are equal */
  rf->rfpulse.nom_pw         =  rf->rfwave.duration;
  rf->rfpulse.nom_bw         =  rf->bw;
  rf->rfpulse.activity       =  PSD_PULSE_OFF; /* is set to PSD_SCAN_ON by ks_pg_rf() */
  rf->rfpulse.reference      =  0;
  rf->rfpulse.isodelay       =  rf->iso2end;
  rf->rfpulse.scale          =  1.0;
  if (rf->rfpulse.extgradfile != 1) {
    rf->rfpulse.extgradfile =  0; /* Only allow 0 or 1. Disallows RF pulse stretching if 1 */
  }

  return SUCCESS;
}




STATUS ks_eval_rf(KS_RF *rf, const char * const desc) {

  if (desc == NULL || desc[0] == ' ') {
    return ks_error("ks_eval_rf: description (2nd arg) cannot be NULL or begin with a space");
  } else {
    strncpy(rf->rfwave.description, desc, KS_DESCRIPTION_LENGTH / 2); /* truncate after half max # chars to allow for suffixes */
  }

  ks_eval_rf_relink(rf);

  if (rf->role <= KS_RF_ROLE_NOTSET || rf->role > KS_RF_ROLE_INV) {
    return ks_error("ks_eval_rf(%s): 'rf.role' is not set or is invalid", desc);
  }
  if (rf->flip <= 0.0 || rf->flip > 10000.0) {
    return ks_error("ks_eval_rf(%s): 'rf.flip' (%.2f) must be in the range (0,3000] deg", desc, rf->flip);
  }
  if (rf->bw < 0 || rf->bw > 100000) {
    return ks_error("ks_eval_rf(%s): 'rf.bw' (%.2f)  must be in the range [0,100000]", desc, rf->bw);
  }
  /* check that RF duration is even */
  if (rf->rfwave.duration % 2)
    return ks_error("ks_eval_rf (%s): RF duration (%d) [us] must be even", rf->rfwave.description, rf->rfwave.duration);

  /* check that RF duration and optional THETA / OMEGA waveforms have same duration */
  if (rf->omegawave.res) {
    sprintf(rf->omegawave.description, "%s", rf->rfwave.description); /* suffix '.x' is added automatically in ks_pg_wave(), where 'x' is the 1st letter of the board */
    if (rf->rfwave.duration != rf->omegawave.duration)
      return ks_error("ks_eval_rf (%s): OMEGA duration (%d) [us] must equal RF duration (%d) [us]", rf->rfwave.description, rf->omegawave.duration, rf->rfwave.duration);
  }
  if (rf->thetawave.res) {
    sprintf(rf->thetawave.description, "%s", rf->rfwave.description);
    if (rf->rfwave.duration != rf->thetawave.duration)
      return ks_error("ks_eval_rf (%s): THETA duration (%d) [us] must equal RF duration (%d) [us]", rf->rfwave.description, rf->thetawave.duration, rf->rfwave.duration);
  }

  if (rf->iso2end < 0 || rf->iso2end > rf->rfwave.duration) {
    return ks_error("ks_eval_rf (%s): 'rf.iso2end (%d) must in range [0,%d] [us]", rf->rfwave.description, rf->iso2end, rf->rfwave.duration);
  }

  /* check that .iso2end is equal to .rfpulse.isodelay */
  if (rf->rfpulse.isodelay != rf->iso2end) {
    return ks_error("ks_eval_rf (%s): 'rf.rfpulse.isodelay (%d) must be equal to 'rf.iso2end' (%d) [us]", rf->rfwave.description, rf->rfpulse.isodelay, rf->iso2end);
  }

  rf->rfpulse.isodelay = RUP_GRD(rf->rfpulse.isodelay); /* Round up using RUP_GRD() to fall on gradient raster (GRAD_UPDATE_TIME = 4us) */
  rf->iso2end = rf->rfpulse.isodelay;
  rf->start2iso = rf->rfwave.duration - rf->iso2end; /* time from start of RF pulse to its magnetic center */

  /* initialize amplitude to 1 */
  rf->amp = 1;

  /* set the usage of the RF PULSE to off until .base.ninst > 0 (see ks_pg_rf() on HOST) */
  rf->rfpulse.activity = PSD_PULSE_OFF;

  /* initialize number of occurrences of RF pulse to zero */
  rf->rfpulse.num = 0;


  return SUCCESS;

} /* ks_eval_rf */



void ks_eval_rf_relink(KS_RF *rf) {

  /* link as required by RF_PULSE */
  rf->rfpulse.pw = &(rf->rfwave.duration);
  rf->rfpulse.amp = &(rf->amp);
  rf->rfpulse.act_fa = &(rf->flip);
  rf->rfpulse.res = &(rf->rfwave.res);
  rf->rfpulse.exciter = &ks_rhoboard;
}




/*******************************************************************************************************
 *  Slice selective RF object (KS_SELRF)
 *******************************************************************************************************/



STATUS ks_eval_seltrap(KS_TRAP *trap, const char * const desc, float slewrate, float slthick, float bw, int rfduration) {

  /* Reset all fields and waveforms */
  ks_init_trap(trap);

  if ((rfduration < GRAD_UPDATE_TIME) || (rfduration % GRAD_UPDATE_TIME)) {
    return ks_error("%s: RF duration must be positive and divisible by 4", __FUNCTION__);
  }

  if (slthick < 0 || bw < 0) {
    /* N.B.: slthick may be exactly 0, in which case a zero amp gradient is created */
    return ks_error("%s: slice thickness & bandwidth cannot be negative", __FUNCTION__);
  }

  if (desc == NULL || desc[0] == ' ') {
    return ks_error("ks_eval_seltrap: trap name (2nd arg) cannot be NULL or begin with a space");
  }

  strncpy(trap->description, desc, KS_DESCRIPTION_LENGTH - 1);

  trap->amp = ks_calc_selgradamp(bw, slthick); /* amp [G/cm] */

  trap->ramptime = RUP_GRD((int) (trap->amp / slewrate)); /* ramp time [us] */

  if (trap->ramptime <= 0) {
    trap->ramptime = 4;
  }

  trap->plateautime = rfduration; /* plateau time [us] */
  trap->duration = trap->plateautime + 2 * trap->ramptime; /* total duration [us] */
  trap->area = (trap->plateautime + trap->ramptime) * trap->amp;


 return SUCCESS;

}




STATUS ks_eval_selwave(KS_WAVE *gradwave, const char * const desc, float slthick, float bw, int rfduration) {

  /* Reset all fields and waveforms */
  ks_init_wave(gradwave);

  if ((rfduration < GRAD_UPDATE_TIME) || (rfduration % GRAD_UPDATE_TIME)) {
    return ks_error("%s: RF duration must be positive and divisible by 4", __FUNCTION__);
  }

  if (slthick < 0 || bw < 0) {
    /* N.B.: slthick may be exactly 0, in which case a zero amp gradient is created */
    return ks_error("%s: slice thickness & bandwidth cannot be negative", __FUNCTION__);
  }

  if (desc == NULL || desc[0] == ' ') {
    return ks_error("ks_eval_selwave: wave name (2nd arg) cannot be NULL or begin with a space");
  }

  strncpy(gradwave->description, desc, KS_DESCRIPTION_LENGTH - 1);

  float amp = ks_calc_selgradamp(bw, slthick); /* amp [G/cm] */
  gradwave->duration = rfduration; /* total duration [us] */
  gradwave->res = rfduration / GRAD_UPDATE_TIME;

  int i;
  for (i=0; i<gradwave->res; i++) {
    gradwave->waveform[i] = amp;
  }

  gradwave->area = ks_wave_sum(gradwave);
  gradwave->gradwave_units = KS_GRADWAVE_ABSOLUTE;

 return SUCCESS;

}




STATUS ks_eval_selrf_constrained(KS_SELRF *selrf, const char * const desc, float ampmax, float slewrate) {
  char selname[KS_DESCRIPTION_LENGTH], preselname[KS_DESCRIPTION_LENGTH], postselname[KS_DESCRIPTION_LENGTH];
  float minthick;
  STATUS status;

  if (desc == NULL || desc[0] == ' ') {
    return ks_error("%s: description (2nd arg) cannot be NULL or begin with a space", __FUNCTION__);
  }

  /* setup the RF pulse */
  status = ks_eval_rf(&selrf->rf, desc);
  KS_RAISE(status);

  /* with slice selection (trap or gradwave), the RF duration must be divisible by GRAD_UPDATE_TIME (4) [us] */
  if ((selrf->rf.rfwave.duration < 4) || (selrf->rf.rfwave.duration % 4)) {
    return ks_error("%s (%s): RF duration (%d) must be divisible by GRAD_UPDATE_TIME (4)", __FUNCTION__, desc, selrf->rf.rfwave.duration);
  }

  /* init members of selrf to their default state */
  ks_init_trap(&selrf->pregrad);
  ks_init_trap(&selrf->grad);
  ks_init_trap(&selrf->postgrad);
  ks_init_sms_info(&selrf->sms_info);

  /* Slice thickness */
  /* protect against e.g. +/-Inf thicknesses. Note that we do *NOT* complain about slthick = 0. This is OK and means zero amp dummy gradient */
  if (selrf->slthick < 0 || selrf->slthick > 600) {
    return ks_error("%s (%s): Thickness [%f] not in range [0,600] mm", __FUNCTION__, desc, selrf->slthick);
  }

  minthick = selrf->rf.bw  / (ampmax * (0.1) * GAM);
  if (selrf->slthick > 0 && selrf->slthick < minthick) {
    /* N.B.: checking for slthick > 0 above, allows slthick = 0 without leaading to this error (meaning no slice sel amp) */
    return ks_error("%s (%s): Minimum thickness violation (desired - %.2f [mm] min - %.2f [mm])", __FUNCTION__, desc, selrf->slthick, minthick);
  }

  /* If bridged crushers are wanted, add a constant gradwave as slice slelection */
  if (selrf->rf.role == KS_RF_ROLE_REF && selrf->bridge_crushers && selrf->gradwave.res == 0) {
    ks_init_wave(&selrf->gradwave);
    ks_eval_selwave(&selrf->gradwave, selname, selrf->slthick, selrf->rf.bw, selrf->rf.rfwave.duration); 
    KS_RAISE(status);
  }

  /* if we have a gradwave generated, check that it has the same duration as the rfpulse */
  if (selrf->gradwave.res) {
    if (selrf->gradwave.duration != selrf->rf.rfwave.duration) {
      return ks_error("%s (%s): Custom gradient wave duration (%d) must equal the RF duration (%d)", __FUNCTION__, desc, selrf->gradwave.duration, selrf->rf.rfwave.duration);
    }
    if (selrf->gradwave.duration % selrf->gradwave.res) {
      return ks_error("%s (%s): Custom gradient wave duration (%d) must be divisible by res (%d)", __FUNCTION__, desc, selrf->gradwave.duration, selrf->gradwave.res);
    }
    ks_create_suffixed_description(selrf->gradwave.description, selrf->rf.rfwave.description, ".gradwave");
  }

  /* Setup the gradients (trapezoid case) */
  ks_create_suffixed_description(selname, selrf->rf.rfwave.description, "_trap"); /* suffix '.x' is added automatically in ks_pg_wave(), where 'x' is the 1st letter of the board , ".trap"*/


  /* slice select */
  if (selrf->gradwave.res == 0) {
    /* slice select using trapezoid (.grad). If selrf->slthick = 0 or selrf->rf.bw = 0, a zero amp gradient will be created to make consistent KS_SELRF
       This is facilitated by ks_eval_seltrap()->ks_calc_selgradamp() */
    status = ks_eval_seltrap(&selrf->grad, selname, slewrate, selrf->slthick, selrf->rf.bw, selrf->rf.rfwave.duration);
    KS_RAISE(status);
  } else {
    if (selrf->gradwave.gradwave_units == KS_GRADWAVE_RELATIVE) {
      /* slice select using existing custom waveform (.gradwave) - scaling gradwave amplitude correctly regardless of current amplitude
       assumes that the maximum absolute value in gradwave.waveform corresponds to the gradient amplitude intended for the current rf BW and slthick */
      float maxval = ks_wave_max(&selrf->gradwave); /* not absmax, but max, to allow for larger negative lobes for e.g. flyback SPSP */
      if (maxval <= 0) {
        /* a gradwave that is strictly below zero, normalize instead by absmax */
        maxval = fabs(ks_wave_min(&selrf->gradwave));
      }
      ks_wave_multiplyval(&selrf->gradwave, ks_calc_selgradamp(selrf->rf.bw, selrf->slthick) / maxval);

    } else if (selrf->gradwave.gradwave_units == KS_NOTSET) {
      return ks_error("%s (%s): Custom gradient wave scaling policy not set", __FUNCTION__, desc);
    }

    ks_wave_compute_params(&selrf->gradwave);
  }


  switch (selrf->rf.role) {

  /********************************************************************************/
  /*********************** RF SEL EXCITATION **************************************/
  /********************************************************************************/
  case KS_RF_ROLE_EXC: {

    int gradwave_ramp_duration = 0;

    ks_create_suffixed_description(postselname, selrf->rf.rfwave.description, ".reph");
    if (selrf->gradwave.res == 0) { /* Slice selection using .grad (KS_TRAP) */

      /* area needed for rephaser */
      selrf->postgrad.area = -(selrf->rf.rfpulse.isodelay + selrf->grad.ramptime / 2) * selrf->grad.amp;

    } else { /* Slice selection using .gradwave (KS_WAVE) */

      int i, isostart;
      int grad_dwell = selrf->gradwave.duration / selrf->gradwave.res;

      /* if gradwave starts at non-zeoro, add ramps */
      if (selrf->gradwave.waveform[0] > 0) {

        if (areSame(selrf->gradwave.waveform[selrf->gradwave.res - 1], 0)) {
          return ks_error("%s: exc-gradwave starts at non-zero and ends at zero",__FUNCTION__);
        }

        KS_WAVE ramp;
        status = ks_eval_ramp(&ramp, slewrate*0.99, selrf->gradwave.waveform[0], grad_dwell);
        KS_RAISE(status);

        /* Append ramp to slice selective gradwave (including some mirror gymnastics) */
        status = ks_eval_mirrorwave(&ramp);
        KS_RAISE(status);

        status = ks_eval_mirrorwave(&selrf->gradwave);
        KS_RAISE(status);

        status = ks_eval_append_two_waves(&selrf->gradwave, &ramp);
        KS_RAISE(status);

        status = ks_eval_mirrorwave(&selrf->gradwave);
        KS_RAISE(status);

        status = ks_eval_append_two_waves(&selrf->gradwave, &ramp);
        KS_RAISE(status);

        /* calculate usefull params */
        ks_wave_compute_params(&selrf->gradwave);
        gradwave_ramp_duration = ramp.duration;
      }

      /* rephaser area needed for gradient wave */
      selrf->postgrad.area = 0;
      if (selrf->rf.iso2end_subpulse != KS_NOTSET) {
        /* SPSP's subpulses may be linear, min or max phase pulses, not neccesarily
           centered on the plateau of each lobe. For these pulses, the iso2end_subpulse field
           is not KS_NOTSET (-1). One can also have self-rephased gradwaves, not needing
           a .postgrad. In this case rf.iso2end_subpulse = 0 would disable this, while keeping
           rf.iso2end for TE calculations */
        isostart = (selrf->gradwave.duration - selrf->rf.iso2end_subpulse) / grad_dwell;
      } else {
        isostart = (selrf->gradwave.duration - selrf->rf.iso2end) / grad_dwell;
      }
      isostart -= gradwave_ramp_duration / grad_dwell;
      for (i = isostart; i < selrf->gradwave.res; i++) {
        selrf->postgrad.area -= selrf->gradwave.waveform[i] * grad_dwell;
      }
    }

    /* add off-set area */
    selrf->postgrad.area += selrf->postgrad_area_offset;

    /* setup rephaser trapezoid */
    status = ks_eval_trap_constrained(&selrf->postgrad, postselname, ampmax, slewrate, 0);
    KS_RAISE(status);

    selrf->grad2rf_start = selrf->grad.ramptime + gradwave_ramp_duration;
    selrf->rf2grad_end = selrf->postgrad.duration + selrf->grad.ramptime + gradwave_ramp_duration;

    break;
  }
  /********************************************************************************/
  /*********************** RF SEL REFOCUSING **************************************/
  /********************************************************************************/
  case KS_RF_ROLE_REF: {
    float grad_area = (selrf->gradwave.res==0) ? selrf->grad.area : selrf->gradwave.area;

    /* area [(G/cm)*us], GAM [Hz/G], slthick [mm] */
    const float slthick = selrf->slthick > 0 ? selrf->slthick : 10; /* dafault to 10mm if not selective */
    float crusher_area;
    if (areSame(selrf->crusher_dephasing, -1.0)) { /* flowcomp z negative crushers using slicesel area */
      crusher_area = -grad_area; /* negative flowcomp z crushers */
    } else {
      crusher_area = ks_cycles_to_area(selrf->crusher_dephasing, slthick);
      crusher_area -= grad_area / 2.0;
      if (crusher_area < 0.0) {crusher_area = 0.0;}
    }

    float pregrad_area = crusher_area + selrf->pregrad_area_offset;
    float postgrad_area = crusher_area + selrf->postgrad_area_offset;


    if (selrf->gradwave.res==0) {
      /* KS_TRAP crushers (non-bridged) */
      selrf->pregrad.area = pregrad_area;
      selrf->postgrad.area = postgrad_area;

      ks_create_suffixed_description(preselname, selrf->rf.rfwave.description, ".LC");
      ks_create_suffixed_description(postselname, selrf->rf.rfwave.description, ".RC");

      status = ks_eval_trap_constrained(&selrf->pregrad,  preselname, ampmax, slewrate, 0);
      KS_RAISE(status);
      status = ks_eval_trap_constrained(&selrf->postgrad, postselname, ampmax, slewrate, 0);
      KS_RAISE(status);

      selrf->grad2rf_start = selrf->pregrad.duration + selrf->grad.ramptime;
      selrf->rf2grad_end = selrf->postgrad.duration + selrf->grad.ramptime;

    } else {
      /* KS_WAVE crushers (bridged) */
      ks_crusher_constraints crusher_constraints = KS_INIT_CRUSHER_CONSTRAINT;
      crusher_constraints.ampmax = ampmax;
      crusher_constraints.slew = slewrate;
      crusher_constraints.min_duration = 0;
      crusher_constraints.strategy = KS_CRUSHER_STRATEGY_EXACT;
      KS_WAVE precrusher;
      KS_WAVE postcrusher;

      const float start_amp = 1.5f * selrf->gradwave.waveform[0] - 0.5f * selrf->gradwave.waveform[1];
      crusher_constraints.area = pregrad_area;
      status = ks_eval_crusher(&precrusher,start_amp, crusher_constraints);
      KS_RAISE(status);

      const float end_amp = 1.5f * selrf->gradwave.waveform[selrf->gradwave.res - 1] - 0.5f * selrf->gradwave.waveform[selrf->gradwave.res - 2];
      crusher_constraints.area = postgrad_area;
      status = ks_eval_crusher(&postcrusher, end_amp, crusher_constraints);
      KS_RAISE(status);

      /* Append crushers to slice selective gradwave (including some mirror gymnastics) */
      status = ks_eval_mirrorwave(&selrf->gradwave);
      KS_RAISE(status);

      status = ks_eval_mirrorwave(&precrusher);
      KS_RAISE(status);

      status = ks_eval_append_two_waves(&selrf->gradwave, &precrusher);
      KS_RAISE(status);

      status = ks_eval_mirrorwave(&selrf->gradwave);
      KS_RAISE(status);

      status = ks_eval_mirrorwave(&postcrusher);
      KS_RAISE(status);

      status = ks_eval_append_two_waves(&selrf->gradwave, &postcrusher);
      KS_RAISE(status);

      /* calculate usefull params */
      ks_wave_compute_params(&selrf->gradwave);
      selrf->grad2rf_start = precrusher.duration;
      selrf->rf2grad_end = postcrusher.duration;

    }

    break;
  }
  /********************************************************************************/
  /*********************** RF SEL INVERSION ***************************************/
  /********************************************************************************/
  case KS_RF_ROLE_INV:

    if (selrf->gradwave.res!=0) {
      return ks_error("%s(%s): KS_RF_ROLE_INV does not support gradwaves yet.",__FUNCTION__,desc);
    }

    selrf->grad2rf_start = selrf->grad.ramptime;
    selrf->rf2grad_end = selrf->grad.ramptime;

    break;
  /********************************************************************************/
  /*********************** RF SPATIAL SATURATION **********************************/
  /********************************************************************************/
  case KS_RF_ROLE_SPSAT:

    if (selrf->gradwave.res!=0) {
      return ks_error("%s(%s): KS_RF_ROLE_SPSAT does not support gradwaves yet.",__FUNCTION__,desc);
    }

    selrf->grad2rf_start = selrf->grad.ramptime;
    selrf->rf2grad_end = selrf->grad.ramptime;

    break;
  /********************************************************************************/
  /********************************************************************************/
  /********************************************************************************/
  default:
    return ks_error("ks_eval_selrf(%s): RF role must be one of: KS_RF_ROLE_EXC, KS_RF_ROLE_REF, KS_RF_ROLE_INV, KS_RF_ROLE_SPSAT", desc);
    break;

  } /* role */



  /********************************************************************************/
  /*********************** Validation *********************************************/
  /********************************************************************************/

  if (selrf->gradwave.res) { /* custom gradient wave */
    float mostfavorable_gradmax = FMax(3, loggrd.tx, loggrd.ty, loggrd.tz);

    if (ks_wave_absmax(&selrf->gradwave) > mostfavorable_gradmax) {
      return ks_error("ks_eval_selrf(%s): too large gradient amplitude for 'gradwave'", desc);
    }

    /* we explicitly check for the hardware limits since loggrd is typically lower because obloptimze_epi 
    (which allows access to the full slewrate limits) was not called for the rf pulse  */

    /* we use config variables because loggrd or physgrd may be derated for silent scanning purposes selrf waves are timing sensitive */
    extern float cfxfs, cfyfs, cfzfs;
    extern int cfrmp2xfs, cfrmp2yfs, cfrmp2zfs;
    float max_sys_slewrate = FMin(3, cfxfs, cfyfs, cfzfs)/IMax(3,cfrmp2xfs,cfrmp2yfs,cfrmp2zfs);
    float max_gradwave_slewrate = ks_wave_maxslew(&selrf->gradwave);
    if (max_gradwave_slewrate > 1.00001 * max_sys_slewrate) {
        return ks_error("ks_eval_selrf(%s): slewrate of gradwave (%.1f T/m/s) exceeds limits (%.1f T/m/s)", desc, max_gradwave_slewrate*1e4, max_sys_slewrate*1e4);
    }

    } else { /* standard trapezoid */

    /* timing check: grad vs. RF */
    if (selrf->grad.duration && selrf->grad.plateautime != selrf->rf.rfwave.duration) {
      return ks_error("ks_eval_selrf(%s): grad.plateautime (%d) != rf.rfwave.duration (%d)", desc, selrf->grad.plateautime, selrf->rf.rfwave.duration);
    }

    /* gradient duration divisibility check */
    if (selrf->grad.duration % GRAD_UPDATE_TIME) {
      return ks_error("ks_eval_selrf(%s): grad.duration (%d) must be divisible by 4 [us]", desc, selrf->grad.duration);
    }

  }

  if (selrf->rf.omegawave.res) {
    ks_wave_compute_params(&selrf->rf.omegawave);
    float grad_absmax;
    if (selrf->gradwave.res) {
      ks_wave_compute_params(&selrf->gradwave);
      grad_absmax = selrf->gradwave.abs_max_amp;
    } else if (selrf->grad.duration) {
      grad_absmax = fabs(selrf->grad.amp);
    } else {
      return ks_error("ks_eval_selrf(%s): no grads for the omega, this shouldn't be possible", desc);
    }
    ks_wave_multiplyval(&selrf->rf.omegawave, grad_absmax / selrf->rf.omegawave.abs_max_amp * GAM / 10.0);
    selrf->rf.omegawave.fs_factor = 0.1; /* so 1 bit of iamp = 0.1 mm */
  }

  return SUCCESS;

} /* ks_eval_selrf_constrained */



STATUS ks_eval_selrf(KS_SELRF *selrf, const char * const desc) {

  return ks_eval_selrf_constrained(selrf, desc, ks_syslimits_ampmax(loggrd), ks_syslimits_slewrate(loggrd));

}




STATUS ks_eval_selrf2(KS_SELRF *selrf, const char * const desc) {

  return ks_eval_selrf_constrained(selrf, desc, ks_syslimits_ampmax2(loggrd), ks_syslimits_slewrate2(loggrd));

}




STATUS ks_eval_selrf1(KS_SELRF *selrf, const char * const desc) {

  return ks_eval_selrf_constrained(selrf, desc, ks_syslimits_ampmax1(loggrd), ks_syslimits_slewrate1(loggrd));

}




STATUS ks_eval_selrf1p(KS_SELRF *selrf, const char * const desc) {

  return ks_eval_selrf_constrained(selrf, desc, ks_syslimits_ampmax1p(loggrd), ks_syslimits_slewrate1p(loggrd));

}




float ks_eval_findb1(KS_SELRF *selrf, float max_b1, double scaleFactor, int sms_multiband_factor, int sms_phase_modulation_mode, float sms_slice_gap) {

  STATUS status;
  float curr_b1 = 0.0;
  int debug = 0;
  double stepDown = 1.01;
  double stepUp = 1.05;

  /* Init tmp rf object */
  KS_SELRF selrftmp;
  ks_init_selrf(&selrftmp);

  /* Make the RF-pulse longer until its B1 peak is under its target */
  do {
    selrftmp = *selrf;
    ks_eval_stretch_rf(&selrftmp.rf, scaleFactor);
    status = ks_eval_selrf(&selrftmp, "tmp+");
    if (status == FAILURE) {continue;}
    if (sms_multiband_factor > 1) {
      status = ks_eval_sms_make_multiband(&selrftmp, &selrftmp, sms_multiband_factor, sms_phase_modulation_mode, sms_slice_gap, 0);
    } else {
      status = ks_eval_rfstat(&selrftmp.rf);
    }
    if (status == FAILURE) {continue;}
    curr_b1 = selrftmp.rf.rfpulse.max_b1;
    scaleFactor *= stepUp;
    if (debug) {ks_dbg("+ scaleFactor=%f, currB1=%f, maxB1=%f", scaleFactor, curr_b1, max_b1);}
  }
  while (fabs(curr_b1) > max_b1);
  scaleFactor /= stepUp;

  /* Make the RF-pulse shorter until its B1 peak hits its target */
  while ((fabs(curr_b1) < max_b1) && (scaleFactor > 0)) {
    selrftmp = *selrf;
    ks_eval_stretch_rf(&selrftmp.rf, scaleFactor);
    status = ks_eval_selrf(&selrftmp, "tmp-");
    if (status == FAILURE) {break;}
    if (sms_multiband_factor > 1) {
      status = ks_eval_sms_make_multiband(&selrftmp, &selrftmp, sms_multiband_factor, sms_phase_modulation_mode, sms_slice_gap, 0);
    } else {
      status = ks_eval_rfstat(&selrftmp.rf);
    }
    if (status == FAILURE) {break;}
    curr_b1 = selrftmp.rf.rfpulse.max_b1;
    scaleFactor /= stepDown;
    if (debug) {ks_dbg("- scaleFactor=%f, currB1=%f, maxB1=%f", scaleFactor, curr_b1, max_b1);}
  }
  scaleFactor *= stepDown;

  return scaleFactor;
}




void ks_eval_transient_SPGR_FA_train_recursive(float* FA_train, float* MZ_train, int N, float E1, float target_MT, int n) {
  float MT;
  n = (n < 0) ? (N-1) : n;
  if (n == 0) {
    MZ_train[n] = 1.0;
    FA_train[n] = asin(target_MT);
  } else {
    ks_eval_transient_SPGR_FA_train_recursive(FA_train, MZ_train, N, E1, target_MT, n-1);
    MT = MZ_train[n-1] * sin(FA_train[n-1]);
    MZ_train[n] = MZ_train[n-1] * cos(FA_train[n-1]) * E1 + 1.0 - E1;
    FA_train[n] = asin( MT / MZ_train[n] );
  }
}




void ks_eval_transient_SPGR_FA_train_binary_search(float* FA_train, float* MZ_train, int N, float E1, float MTlo, float MThi, float total_FA) {
  float thresh = 1e-7;
  if ((MThi-MTlo) < thresh) {
    return ks_eval_transient_SPGR_FA_train_recursive(FA_train, MZ_train, N, E1, MTlo, -1);
  }
  float MT = (MTlo + MThi)/2.0;
  ks_eval_transient_SPGR_FA_train_recursive(FA_train, MZ_train, N, E1, MT, -1);
  if (ks_eval_check_FA_train(N, FA_train, MZ_train, total_FA) == FAILURE) {
    return ks_eval_transient_SPGR_FA_train_binary_search(FA_train, MZ_train, N, E1, MTlo, MT, total_FA); /* Less greed */
  } else {
    return ks_eval_transient_SPGR_FA_train_binary_search(FA_train, MZ_train, N, E1, MT, MThi, total_FA); /* More greed */
  }
}




STATUS ks_eval_check_FA_train(int N, float* FA_train, float* MZ_train, float total_FA) {
  if (isnan(FA_train[N-1])) {return FAILURE;} /* infeasible solution */
  if (acos(MZ_train[N-1] * cos(FA_train[N-1])) > total_FA * PI/180.0) {return FAILURE;} /* total FA exceeded */
  int n;
  for (n=0; n<N; n++) {
    if ((n > 0) && (FA_train[n] < FA_train[n-1])) {return FAILURE;} /* decreasing FA */
    if ((n < N-1) && (FA_train[n] > 89.0 * PI/180.0)) {return FAILURE;} /* only last FA may be 90 degrees */
  }
  return SUCCESS;
}




STATUS ks_eval_transient_SPGR_FA_train(float* FA_train, int N, float TR, float T1, float total_FA) {
  if (N <= 0) {return ks_error("%s: N must be >0, but is %d", __FUNCTION__, N);}
  float E1 = exp(-TR/T1);
  float MTlo = 0.0;
  float MThi = 1.0;
  float MZ_train[N];
  ks_eval_transient_SPGR_FA_train_binary_search(FA_train, MZ_train, N, E1, MTlo, MThi, total_FA);
  if (ks_eval_check_FA_train(N, FA_train, MZ_train, total_FA) == FAILURE) {
    return ks_error("Ops");
  } else {
    int n;
    for (n=0; n<N; n++) {FA_train[n]*=180.0/PI;} /* radians->degrees */
    return SUCCESS;
  }
}




STATUS ks_eval_sms_make_multiband(KS_SELRF *selrfMB, const KS_SELRF *selrf, const int sms_multiband_factor,
                                  const int sms_phase_modulation_mode, const float sms_slice_gap, int debug) {

  int jdx, idx;
  float sms_phase_modulation[16] = KS_INITZEROS(16);
  STATUS status;

  /* Debug stuff */
  int single_band_mode = -1; /* -1 = off */
  char fname[KS_DESCRIPTION_LENGTH + 64];
  char outputdir_uid[250];
  FILE* asciiWaveReal = NULL;
  FILE* asciiWaveImag = NULL;
  FILE* asciiWaveRealMB = NULL;
  FILE* asciiWaveImagMB = NULL;
  FILE* asciiWaveRealMod = NULL;
  FILE* asciiWaveImagMod = NULL;
  FILE* asciiWaveParams = NULL;

  if (debug) {

#ifdef SIM
    sprintf(outputdir_uid, "./");
#else
    char cmd[250];
    snprintf(outputdir_uid, 250, "/usr/g/mrraw/kstmp/%010d/", rhkacq_uid);
    snprintf(cmd, 250, "mkdir -p %s > /dev/null", outputdir_uid);
    system(cmd);
#endif

    snprintf(fname, KS_DESCRIPTION_LENGTH + 64, "%s%s_mb_rf_Real.txt", outputdir_uid, selrfMB->rf.rfwave.description);
    asciiWaveReal = fopen(fname,"w");
    snprintf(fname, KS_DESCRIPTION_LENGTH + 64, "%s%s_mb_rf_Imag.txt", outputdir_uid, selrfMB->rf.rfwave.description);
    asciiWaveImag = fopen(fname,"w");
    snprintf(fname, KS_DESCRIPTION_LENGTH + 64, "%s%s_mb_rfMB_Real.txt", outputdir_uid, selrfMB->rf.rfwave.description);
    asciiWaveRealMB = fopen(fname,"w");
    snprintf(fname, KS_DESCRIPTION_LENGTH + 64, "%s%s_mb_rfMB_Imag.txt", outputdir_uid, selrfMB->rf.rfwave.description);
    asciiWaveImagMB = fopen(fname,"w");
    snprintf(fname, KS_DESCRIPTION_LENGTH + 64, "%s%s_mod_rfMB_Real.txt", outputdir_uid, selrfMB->rf.rfwave.description);
    asciiWaveRealMod = fopen(fname,"w");
    snprintf(fname, KS_DESCRIPTION_LENGTH + 64, "%s%s_mod_rfMB_Imag.txt", outputdir_uid, selrfMB->rf.rfwave.description);
    asciiWaveImagMod = fopen(fname,"w");
  }

  /* Check if gradient is present */
  if (areSame(selrf->grad.amp, 0) && (selrf->gradwave.res == 0)) {
    return ks_error("ks_eval_sms_make_multiband: Need a pre calculated gradient");
  }

  /* Duplicate the selRF structure  */
  if (selrfMB != selrf ) {
    *selrfMB = *selrf;
  }

  /* Set info struct */
  selrfMB->sms_info.mb_factor = sms_multiband_factor;
  selrfMB->sms_info.slice_gap = sms_slice_gap;
  selrfMB->sms_info.pulse_type = KS_SELRF_SMS_MB;

  /* Save max b1 to make sure it does not get overwritten */
  double base_rf_max_amp = ks_waveform_absmax(selrf->rf.rfwave.waveform, ks_wave_res(&selrf->rf.rfwave));

  /* Get phase modulation to lower peak B1 */
  if (sms_phase_modulation_mode != KS_SELRF_SMS_PHAS_MOD_OFF) {
    status = ks_eval_sms_get_phase_modulation(sms_phase_modulation, sms_multiband_factor, sms_phase_modulation_mode);
    KS_RAISE(status);
  }

  /* initialize thetawave */
  if (selrfMB->rf.thetawave.res == 0) {
    ks_init_wave(&selrfMB->rf.thetawave);
    strncpy(selrfMB->rf.thetawave.description, selrfMB->rf.rfwave.description, KS_DESCRIPTION_LENGTH);
    selrfMB->rf.thetawave.res = selrfMB->rf.rfwave.res;
    selrfMB->rf.thetawave.duration = selrfMB->rf.rfwave.duration;
  }

  if (debug) {sprintf(fname, "%s%s_mb_rf_base.txt", outputdir_uid, selrfMB->rf.rfwave.description); ks_print_waveform(selrf->rf.rfwave.waveform, fname, selrf->rf.rfwave.res); }

  /* Figure out a new resolution and interpolate, since MB pulses require high res */
  int newRes = selrf->rf.rfwave.duration/RF_UPDATE_TIME;
  float *newGrid = (float*)alloca(newRes * sizeof(float));
  for (idx = 0; idx < newRes; idx++) {
      newGrid[idx] = (float) idx / (newRes-1);
  }
  KS_WAVEFORM sb_rf = KS_INIT_WAVEFORM;
  KS_WAVEFORM sb_th = KS_INIT_WAVEFORM;
  if (newRes/selrf->rf.rfwave.res > 1) {
    /* create index for interpolation */
    float *orgGrid = (float*)alloca(selrf->rf.rfwave.res * sizeof(float));
    for (idx = 0; idx < selrf->rf.rfwave.res; idx++) {
      orgGrid[idx] = (float) idx / (selrf->rf.rfwave.res-1);
    }
    /* perform linear interpolation */
    ks_eval_linear_interp1(orgGrid, selrf->rf.rfwave.res, selrf->rf.rfwave.waveform, newGrid, newRes, sb_rf);
    ks_eval_linear_interp1(orgGrid, selrf->rf.thetawave.res, selrf->rf.thetawave.waveform, newGrid, newRes, sb_th);

  } else {
    for (idx=0; idx < newRes; idx++) {
      sb_rf[idx] = selrf->rf.rfwave.waveform[idx]; 
      sb_th[idx] = selrf->rf.thetawave.waveform[idx];
    }
  }

  /* If gradwave is used calc modulation and offset to account for ramps and crushers */
  KS_WAVEFORM kt = KS_INIT_WAVEFORM;
  if (selrf->gradwave.res != 0) {

    /* Find slice selective grad in gradwave */
    int gradWaveOffset_start = 0, gradWaveOffset_end = 0;
    int grad_update_time = selrf->gradwave.duration/selrf->gradwave.res;
    int gradWaveRes = selrf->gradwave.res;
    if (selrf->bridge_crushers) {
      gradWaveOffset_start = selrf->grad2rf_start/grad_update_time;
      gradWaveOffset_end = selrf->gradwave.res - selrf->rf2grad_end/grad_update_time;
    } else {
      int ramp_dur = (selrf->gradwave.duration - selrf->rf.rfwave.duration)/2;
      gradWaveOffset_start = ramp_dur/grad_update_time;
      gradWaveOffset_end = selrf->gradwave.res - gradWaveOffset_start;
    }
    gradWaveRes = gradWaveOffset_end-gradWaveOffset_start;
    KS_WAVEFORM grad = KS_INIT_WAVEFORM;
    for (idx=0; idx < gradWaveRes; idx++) {
      grad[idx] = selrf->gradwave.waveform[gradWaveOffset_start + idx];
    }

    /* interpolate if needed */
    if (grad_update_time/RF_UPDATE_TIME > 1) {
      /* create index for interpolation */
      float *orgGrid = (float*)alloca(gradWaveRes * sizeof(float));
      for (idx = 0; idx < gradWaveRes; idx++) {
        orgGrid[idx] = (float) idx / (gradWaveRes-1);
      }
      /* perform linear interpolation */
      ks_eval_linear_interp1(orgGrid, gradWaveRes, grad, newGrid, newRes, grad);
    }
    if (debug) {sprintf(fname, "%s%s_mb_gradwave.txt", outputdir_uid, selrfMB->rf.rfwave.description); ks_print_waveform(grad, fname, newRes); }

    /* calc kt modulation */
    ks_eval_mirrorwaveform(grad, newRes);
    ks_waveform_cumsum(kt, grad, newRes);

    /* center the modualtion */
    float kt_max = ks_waveform_max(kt, newRes);
    for (idx=0; idx < newRes; idx++) {
      kt[idx] = kt[idx] - kt_max/2.0;
    }
  } else if (debug) {
    KS_WAVEFORM grad = KS_INIT_WAVEFORM;
    for (idx=0; idx < newRes; idx++) {
      grad[idx] = selrf->grad.amp;
    }
    sprintf(fname, "%s%s_mb_gradwave.txt", outputdir_uid, selrfMB->rf.rfwave.description); ks_print_waveform(grad, fname, newRes);
  }

  /* Calc and apply sms modulation to base pulse */
  float *sms_wave_real = (float*)alloca(newRes*sizeof(float));
  float *sms_wave_imag = (float*)alloca(newRes*sizeof(float));  
  for (idx=0; idx < newRes; idx++) {

    double wave_real, wave_imag, sms_modulation_real = 0, sms_modulation_imag = 0, p, slice_offset, wave_grad = 0;

    /* Convert base pulse from polar to cartesian */
    wave_real = sb_rf[idx] * cos(sb_th[idx]*(PI/180.0));
    wave_imag = sb_rf[idx] * sin(sb_th[idx]*(PI/180.0));
    if (debug) {
      fprintf(asciiWaveReal, "%d %f\n", idx, wave_real); fflush(asciiWaveReal);
      fprintf(asciiWaveImag, "%d %f\n", idx, wave_imag); fflush(asciiWaveImag);
    }

    /* Grad */
    if (selrf->gradwave.res == 0) {
      double centeredIndex = idx - (newRes-1) * 0.5;
      wave_grad = selrf->grad.amp * centeredIndex;
    } else {
      wave_grad = kt[idx];
    }

    /* Calc sms modulation */
    for (jdx = 0; jdx < sms_multiband_factor; jdx++) {

      slice_offset = (sms_slice_gap / 10.0) * (1.0 + jdx - (sms_multiband_factor / 2.0) - 0.5);
      p = 2.0 * PI * GAM * slice_offset * wave_grad * RF_UPDATE_TIME * 1e-6 + sms_phase_modulation[jdx];

      if (single_band_mode >= 0) {
        if (jdx == single_band_mode) {
          sms_modulation_real += cos(p);
          sms_modulation_imag += sin(p);
        }
      } else {
        sms_modulation_real += cos(p);
        sms_modulation_imag += sin(p);
      }
    }

    /* Apply sms modulation via complex multiplication */
    sms_wave_real[idx] = wave_real * sms_modulation_real - wave_imag * sms_modulation_imag;
    sms_wave_imag[idx] = wave_real * sms_modulation_imag + wave_imag * sms_modulation_real;

    if (debug) {
      fprintf(asciiWaveRealMB, "%d %f\n", idx, sms_wave_real[idx]); fflush(asciiWaveRealMB);
      fprintf(asciiWaveImagMB, "%d %f\n", idx, sms_wave_imag[idx]); fflush(asciiWaveImagMB);
      fprintf(asciiWaveRealMod, "%d %f\n", idx, sms_modulation_real); fflush(asciiWaveRealMod);
      fprintf(asciiWaveImagMod, "%d %f\n", idx, sms_modulation_imag); fflush(asciiWaveImagMod);
    }

  }

  if (debug) {
    fclose(asciiWaveReal);
    fclose(asciiWaveImag);
    fclose(asciiWaveRealMB);
    fclose(asciiWaveImagMB);
    fclose(asciiWaveRealMod);
    fclose(asciiWaveImagMod);
    /*ks_error("GAM=%g, PI=%g, grad.amp=%g, slice_gap=%g, dt=%d", GAM, PI, selrf->grad.amp, sms_slice_gap, RF_UPDATE_TIME);*/
  }

  for (idx=0; idx < newRes; idx++) {

    if (sms_phase_modulation_mode == KS_SELRF_SMS_PHAS_MOD_OFF || sms_phase_modulation_mode == KS_SELRF_SMS_PHAS_MOD_AMPL) {

      /* Use real part and place waves in selrfMB */
      selrfMB->rf.rfwave.waveform[idx] = sms_wave_real[idx];

    } else {

      /* Convert from polar to cartesian and place waves in selrfMB */
      selrfMB->rf.rfwave.waveform[idx]    = sqrt(pow(sms_wave_real[idx], 2.0) + pow(sms_wave_imag[idx], 2.0));
      selrfMB->rf.thetawave.waveform[idx] = atan2(sms_wave_imag[idx], sms_wave_real[idx]) * (180.0/PI);

    }
  }

  /* Update resolution fields */
  selrfMB->rf.rfwave.res = newRes;
  if (sms_phase_modulation_mode == KS_SELRF_SMS_PHAS_MOD_OFF || sms_phase_modulation_mode == KS_SELRF_SMS_PHAS_MOD_AMPL) {
    selrfMB->rf.thetawave.res = 0;
    selrfMB->rf.thetawave.duration = 0;
  } else {
    selrfMB->rf.thetawave.res = newRes;
  }

  if (debug) {sprintf(fname, "%s%s_mb_rf.txt", outputdir_uid, selrfMB->rf.rfwave.description); ks_print_waveform(selrfMB->rf.rfwave.waveform, fname, selrfMB->rf.rfwave.res); }
  if (debug) {sprintf(fname, "%s%s_mb_theta.txt", outputdir_uid, selrfMB->rf.rfwave.description); ks_print_waveform(selrfMB->rf.thetawave.waveform, fname, selrfMB->rf.thetawave.res); }
  if (debug && selrf->gradwave.res) {sprintf(fname, "%s%s_mb_grad.txt", outputdir_uid, selrfMB->rf.rfwave.description); ks_print_waveform(selrfMB->gradwave.waveform, fname, selrfMB->gradwave.res); }

  /* Modify rfpulse structure */
  double standard_pw = 1e-3;
  double b1_scale_factor = ks_waveform_absmax(selrfMB->rf.rfwave.waveform, newRes) / base_rf_max_amp;
  selrfMB->rf.rfpulse.max_b1 *= b1_scale_factor;
  selrfMB->rf.rfpulse.area /= b1_scale_factor;
  selrfMB->rf.rfpulse.effwidth /= b1_scale_factor;
  selrfMB->rf.rfpulse.max_int_b1_sq = selrfMB->rf.rfpulse.max_b1 * selrfMB->rf.rfpulse.max_b1 * selrfMB->rf.rfpulse.effwidth * (selrfMB->rf.rfwave.duration) * 1e-6 / standard_pw;
  selrfMB->rf.rfpulse.max_rms_b1 = sqrt(selrfMB->rf.rfpulse.max_int_b1_sq / ((selrfMB->rf.rfwave.duration) * 1e-6) * standard_pw);


  /* Normalize to 1 */
  ks_wave_multiplyval(&selrfMB->rf.rfwave, 1.0 / ks_wave_absmax(&selrfMB->rf.rfwave));

  /* register the RF for later heat calcs and RF scaling */
  char description[KS_DESCRIPTION_LENGTH + 4];
  ks_create_suffixed_description(description, selrfMB->rf.rfwave.description, "_sms");
  status = ks_eval_rf(&selrfMB->rf, description);
  KS_RAISE(status);

  if (debug) {
    sprintf(fname, "%s%s_mb_params.txt", outputdir_uid, selrfMB->rf.rfwave.description);
    asciiWaveParams = fopen(fname,"w");
    fprintf(asciiWaveParams, "%f\n", (float)selrfMB->rf.flip);
    fprintf(asciiWaveParams, "%f\n", (float)selrfMB->grad.amp);
    fprintf(asciiWaveParams, "%f\n", (float)selrfMB->rf.rfpulse.max_b1);
    fprintf(asciiWaveParams, "%f\n", (float)selrfMB->slthick);
    fprintf(asciiWaveParams, "%f\n", (float)sms_slice_gap);
    fprintf(asciiWaveParams, "%f\n", (float)sms_multiband_factor);
    fprintf(asciiWaveParams, "%f\n", (float)b1_scale_factor);
    fflush(asciiWaveParams); fclose(asciiWaveParams);
  }

  return SUCCESS;

} /* EOF */


STATUS ks_eval_sms_get_phase_modulation(float *sms_phase_modulation, const int sms_multiband_factor, const int sms_phase_modulation_mode) {
  /*
  %   Return optimal phases for phase-modulated or amplitude-modulated low-peak power multiband pulses.
  %
  %   sms_phase_modulation: Optimal phases
  %   sms_multiband_factor: Number of bands
  %   sms_phase_modulation_mode: phasmod, amplmod, or quadmod
  %
  %   Adopted for EPIC from Will Grissom's (Vanderbilt University, 2015) MATLAB code by Ola Norbeck (Karolinska Unviversity Hospital, 2015).
  */

  int jdx;

  if (sms_phase_modulation_mode == KS_SELRF_SMS_PHAS_MOD_PHAS) {

    /* Wong's sms_phase_modulation: From E C Wong, ISMRM 2012, p. 2209 */

    if ((sms_multiband_factor < 2) || (sms_multiband_factor > 16)) {
      return ks_error("ks_eval_sms_get_phase_modulation: multiband factor (%d, 2nd arg) must be in range [2,16] for Wong's sms_phase_modulation", sms_multiband_factor);
    }

    float P[15][16] = {
      /* mbf = 2  */  {0.0, 1.571},
      /* mbf = 3  */  {0.0, 0.730, 4.602},
      /* mbf = 4  */  {0.0, 3.875, 5.940, 6.197},
      /* mbf = 5  */  {0.0, 3.778, 5.335, 0.872, 0.471},
      /* mbf = 6  */  {0.0, 2.005, 1.674, 5.012, 5.736, 4.123},
      /* mbf = 7  */  {0.0, 3.002, 5.998, 5.909, 2.624, 2.528, 2.440},
      /* mbf = 8  */  {0.0, 1.036, 3.414, 3.778, 3.215, 1.756, 4.555, 2.467},
      /* mbf = 9  */  {0.0, 1.250, 1.783, 3.558, 0.739, 3.319, 1.296, 0.521, 5.332},
      /* mbf = 10 */  {0.0, 4.418, 2.360, 0.677, 2.253, 3.472, 3.040, 3.974, 1.192, 2.510},
      /* mbf = 11 */  {0.0, 5.041, 4.285, 3.001, 5.765, 4.295, 0.056, 4.213, 6.040, 1.078, 2.759},
      /* mbf = 12 */  {0.0, 2.755, 5.491, 4.447, 0.231, 2.499, 3.539, 2.931, 2.759, 5.376, 4.554, 3.479},
      /* mbf = 13 */  {0.0, 0.603, 0.009, 4.179, 4.361, 4.837, 0.816, 5.995, 4.150, 0.417, 1.520, 4.517, 1.729},
      /* mbf = 14 */  {0.0, 3.997, 0.830, 5.712, 3.838, 0.084, 1.685, 5.328, 0.237, 0.506, 1.356, 4.025, 4.483, 4.084},
      /* mbf = 15 */  {0.0, 4.126, 2.266, 0.957, 4.603, 0.815, 3.475, 0.977, 1.449, 1.192, 0.148, 0.939, 2.531, 3.612, 4.801},
      /* mbf = 16 */  {0.0, 4.359, 3.510, 4.410, 1.750, 3.357, 2.061, 5.948, 3.000, 2.822, 0.627, 2.768, 3.875, 4.173, 4.224, 5.941}
    };

    for (jdx = 0; jdx < sms_multiband_factor; jdx++ ) {
      sms_phase_modulation[jdx] = P[sms_multiband_factor - 2][jdx];
    }

  } else if (sms_phase_modulation_mode == KS_SELRF_SMS_PHAS_MOD_AMPL) {

    /*  Seada's Hermitian sms_phase_modulation: From Seada et. al. Optimized Amplitude Modulated Multiband RF Pulse Design. MRM 2017 */

    if ((sms_multiband_factor < 3) || (sms_multiband_factor > 12)) {
      return ks_error("ks_eval_sms_get_phase_modulation: multiband factor (%d, 2nd arg) must be in range [3,12] for Seada's Hermitian sms_phase_modulation", sms_multiband_factor);
    }

    float P[10][12] = {
      /* mbf = 3  */  {1.2846,  0.0000, -1.2846},
      /* mbf = 4  */  {0.9739,  1.3718, -1.3718, -0.9739},
      /* mbf = 5  */  {1.1572, -0.9931,  0.0000,  0.9931, -1.1572},
      /* mbf = 6  */  {1.6912,  2.8117,  1.1572, -1.1572, -2.8117, -1.6912},
      /* mbf = 7  */  {2.5813, -0.5620,  0.1030,  0.0000, -0.1030,  0.5620, -2.5813},
      /* mbf = 8  */  {2.1118,  0.2199,  1.4643,  1.9914, -1.9914, -1.4643, -0.2199, -2.1118},
      /* mbf = 9  */  {0.4800, -2.6669, -0.6458, -0.4189,  0.0000,  0.4189,  0.6458,  2.6669, -0.4800},
      /* mbf = 10 */  {1.6825, -2.3946,  2.9130,  0.3037,  0.7365, -0.7365, -0.3037, -2.9130,  2.3946, -1.6825},
      /* mbf = 11 */  {1.4050,  0.8866, -1.8535,  0.0698, -1.4940,  0.0000,  1.4940, -0.0698,  1.8535, -0.8866, -1.4050},
      /* mbf = 12 */  {1.7296,  0.4433,  0.7208,  2.1904, -2.1956,  0.9844, -0.9844,  2.1956, -2.1904, -0.7208, -0.4433, -1.7296}
    };

    for (jdx = 0; jdx < sms_multiband_factor; jdx++ ) {
      sms_phase_modulation[jdx] = P[sms_multiband_factor - 3][jdx];
    }


  } else if (sms_phase_modulation_mode == KS_SELRF_SMS_PHAS_MOD_QUAD) {

    /* Grissom's quadratic sms_phase_modulation (unpublished) */

    for (jdx = 0; jdx < sms_multiband_factor; jdx++) {
      sms_phase_modulation[jdx] = pow( (3.4 / sms_multiband_factor) * (float)(1.0 + (float)jdx - ((float)sms_multiband_factor / 2.0) - 0.5), 2.0); /* sms_phase_modulation for each band */
    }

  } else {

    return ks_error("ks_eval_sms_get_phase_modulation: sms_phase_modulation_mode (%d, 3rd arg) is of unrecognized type. Choose phasmod = 1, amplmod = 2, or quadmod = 3", sms_phase_modulation_mode);

  }; /* end of if  */

  return SUCCESS;

} /* EOF */

float ks_eval_sms_calc_slice_gap(int sms_multiband_factor, int nslices, float slthick, float slspace) {

  float gap;

  if (nslices % sms_multiband_factor) {
    ks_error("ks_eval_sms_calc_slice_gap: Number of slices (%d, 3rd arg) must be divisible with the SMS factor (%d, 1st arg)", nslices, sms_multiband_factor);
  }

  /* gap between MB slices mm */
  gap = (float)((nslices + sms_multiband_factor - 1) / sms_multiband_factor) * (float)(slthick + slspace);

  return gap;

} /* EOF */

float ks_eval_sms_calc_slice_gap_from_scan_info(int sms_multiband_factor, int nslices, const SCAN_INFO *org_slice_positions) {

  float gap;

  if (nslices % sms_multiband_factor) {
    ks_error("ks_eval_sms_calc_slice_gap: Number of slices (%d, 3rd arg) must be divisible with the SMS factor (%d, 1st arg)", nslices, sms_multiband_factor);
  }

  gap = fabs(org_slice_positions[nslices/sms_multiband_factor].optloc - org_slice_positions[0].optloc);

  return gap;
}




float ks_eval_sms_calc_caipi_area(int caipi_fov_shift, float sms_slice_gap) {

  if ((sms_slice_gap <= 0.0) || (caipi_fov_shift <= 1)) {
    return 0.0;
  }

  /* Calculate CAIPI-blip area in us*G/cm */
  return ((caipi_fov_shift-1)*(float)PI/(float)caipi_fov_shift) / (2.0*PI*GAM*1e-6 * (sms_slice_gap/10.0));

} /* EOF */


STATUS ks_eval_caipiblip(KS_TRAP* caipiblip, const int caipi_factor, const float sms_slice_gap, const KS_DESCRIPTION seq_desc) {
  KS_DESCRIPTION tmpdesc;

  ks_create_suffixed_description(tmpdesc, seq_desc, "_caipiblip");
  caipiblip->area = ks_eval_sms_calc_caipi_area(caipi_factor, sms_slice_gap);
  caipiblip->fs_factor = 0; /* since the amplitude of a caipi-blip is so low, the fs_factor=0 minimizes potential round of errors while scaling */
  return ks_eval_trap(caipiblip, tmpdesc);
}




int ks_eval_sms_rf_scan_info(SCAN_INFO *sms_slice_positions, const SCAN_INFO *org_slice_positions, int sms_factor, int nslices) {

    if (sms_factor == 1) {
      ks_error("%s: sms_factor = %d. Only works with sms_factor > 1.", __FUNCTION__, sms_factor);
      return KS_NOTSET;
    }

    int sms_nslices = CEIL_DIV(nslices, sms_factor);

    /* Index offset between the org. slice postion and the new one */
    int idx_offset = (sms_nslices) * ((int)ceil((float)sms_factor/2.0) - 1);
    idx_offset += sms_factor % 2 ? 0 : (int)floor((float)nslices / ((float)sms_factor * 2.0));

    /* Allocate new struct and fill with shifted postions */
    memcpy(sms_slice_positions, org_slice_positions, sms_nslices * sizeof(SCAN_INFO));
    int idx;
    for (idx = 0; idx < sms_nslices; idx++) {
      if ((sms_nslices) % 2 && !(sms_factor % 2)) { 
        /* if sms_nslices is odd and sms_factor is even, calc pos between indexes */
        float a = org_slice_positions[idx+idx_offset].optloc;
        float b = org_slice_positions[idx+idx_offset+1].optloc; 
        sms_slice_positions[idx].optloc = a - (a-b)/2.0;
      } else {
        sms_slice_positions[idx].optloc = org_slice_positions[idx+idx_offset].optloc;
      }
    }

    return sms_nslices;
}




STATUS ks_eval_stretch_rf(KS_RF *rf, float stretch_factor) {

  STATUS status;
  int idx;
  int debug = 0;
  int mirror_rfpulse = 0;

  if (areSame(stretch_factor, 1.0)) {
    return SUCCESS;
  }

  mirror_rfpulse = (stretch_factor < 0); /* we are going to mirror the RF pulse after stretching */
  stretch_factor = fabsf(stretch_factor);

  if (stretch_factor < FLT_EPSILON) {
    return ks_error("%s(%s): can not stretch by %f", __FUNCTION__, rf->rfwave.description, stretch_factor);
  }

  /* get original duration/resolution and calculate new duration/resolution */
  int orgDur = rf->rfwave.duration;
  int newDur = RUP_GRD((int)(orgDur * stretch_factor));
  int orgRes = rf->rfwave.res;
  int newRes = newDur/RF_UPDATE_TIME;

  if (newRes > KS_MAXWAVELEN) {
    return ks_error("%s(%s): stretch => too high res (%d)", __FUNCTION__, rf->rfwave.description, newRes);
  }

  /* create index for interpolation */
  float *orgGrid = (float*)alloca(orgRes * sizeof(float));
  float *newGrid = (float*)alloca(newRes * sizeof(float));
  for (idx = 0; idx < orgRes; idx++) {
    orgGrid[idx] = (float) idx / (orgRes-1);
  }
  for (idx = 0; idx < newRes; idx++) {
    newGrid[idx] = (float) idx / (newRes-1);
  }

  if (debug == 1) {ks_print_waveform(rf->rfwave.waveform, "stretch_orgrf.txt", orgRes); }

  /* perform linear interpolation */
  ks_eval_linear_interp1(orgGrid, orgRes, rf->rfwave.waveform, newGrid, newRes, rf->rfwave.waveform);

  if (rf->thetawave.duration > 0) {
    ks_eval_linear_interp1(orgGrid, orgRes, rf->thetawave.waveform, newGrid, newRes, rf->thetawave.waveform);
    rf->thetawave.res = newRes;
    rf->thetawave.duration = newDur;
  }

  if (rf->omegawave.duration > 0) {
    ks_eval_linear_interp1(orgGrid, orgRes, rf->omegawave.waveform, newGrid, newRes, rf->omegawave.waveform);
    rf->omegawave.res = newRes;
    rf->omegawave.duration = newDur;
  }

  if (debug == 1) {ks_print_waveform(rf->rfwave.waveform, "stretch_newrf.txt", newRes); }

  /* set new duration, resolution and bandwidth */
  rf->rfwave.duration = newDur;
  rf->rfwave.res = newRes;
  rf->bw *= ((float)orgDur/newDur);
  rf->rfpulse.isodelay = RUP_FACTOR((int)(rf->rfpulse.isodelay * stretch_factor),RF_UPDATE_TIME);
  rf->iso2end = rf->rfpulse.isodelay;
  rf->start2iso = rf->rfwave.duration - rf->iso2end;

  char description[KS_DESCRIPTION_LENGTH];
  if (mirror_rfpulse) {
    rf->iso2end = rf->start2iso; /* N.B.: start2iso will be updated again in ks_eval_rf() */
    rf->rfpulse.isodelay = rf->start2iso;
    ks_eval_mirrorwave(&rf->rfwave);
    ks_eval_mirrorwave(&rf->thetawave);
    ks_eval_mirrorwave(&rf->omegawave);
    ks_create_suffixed_description(description, rf->rfwave.description, "_stretched_mirrored");
    if (debug == 1) {ks_print_waveform(rf->rfwave.waveform, "stretch_mirror_newrf.txt", newRes); }
  } else {
    ks_create_suffixed_description(description, rf->rfwave.description, "_stretched");
  }
  
  status = ks_eval_rf(rf, description);
  KS_RAISE(status);

  status = ks_eval_rfstat(rf);
  KS_RAISE(status);

  return SUCCESS;
}




STATUS ks_eval_sms_make_pins(KS_SELRF *selrfPINS,
                             const KS_SELRF *selrf,
                             float sms_slice_gap) {

  STATUS status;
  int jdx, idx, kdx = 0, gdx = 0, debug = 0;
  int orgDur = selrfPINS->rf.rfwave.duration;
  KS_TRAP gzblip = KS_INIT_TRAP;

  /* Duplicate the selRF structure  */
  if (selrfPINS != selrf ) {
    *selrfPINS = *selrf;
  }

  /* Set info struct */
  selrfPINS->sms_info.slice_gap = sms_slice_gap;
  selrfPINS->sms_info.pulse_type = KS_SELRF_SMS_PINS;

  ks_init_trap(&selrfPINS->grad);

  if (debug == 1) {ks_print_waveform(selrf->rf.rfwave.waveform, "pins_rfMB_base.txt", selrf->rf.rfwave.res); }

  double kzw = (selrf->rf.bw * selrf->rf.rfwave.duration * 1e-6) / (selrf->slthick / 10); /* 1/cm, width in kz-space we must go */
  int NPulses = (floor(ceil(kzw / (1 / (sms_slice_gap / 10))) / 2) * 2) + 1; /* number of subpulses (odd) */

  /* Init waves */
  KS_WAVEFORM rfLowRes = KS_INIT_WAVEFORM;
  KS_WAVE gzBlipWave = KS_INIT_WAVE;
  float *lowResGrid = (float*)alloca(NPulses * sizeof(float));
  float *baseGrid = (float*)alloca(selrf->rf.rfwave.res * sizeof(float));

  /* create index for interpolation */
  for (idx = 0; idx < selrf->rf.rfwave.res; idx++) {
    baseGrid[idx] = (float) idx / (selrf->rf.rfwave.res - 1);
  }
  for (idx = 0; idx < NPulses; idx++) {
    lowResGrid[idx] = (float) idx / (NPulses - 1);
  }

  /* Interpolate the base rf to get PINS sub pulse amplitudes and high res for MB pulse */
  ks_eval_linear_interp1(baseGrid, selrf->rf.rfwave.res, selrf->rf.rfwave.waveform, lowResGrid, NPulses, rfLowRes);

  if (debug == 1) {ks_print_waveform(rfLowRes, "pins_rfLowRes.txt", NPulses); }

  /* Create blip */
  double gArea = 1 / (sms_slice_gap / 10) / (GAM); /* (g sec)/cm, k-area of each blip */
  gzblip.area = gArea * 1e6; /* (G/cm)*us */
  status = ks_eval_trap1(&gzblip, "PINSblip"); KS_RAISE(status);
  status = ks_eval_trap2wave(&gzBlipWave, &gzblip); KS_RAISE(status);
  ks_init_trap(&gzblip);

  /* Matched-duration RF subpulses */
  float maxB1 = 0.24; /* temp. */
  float dt = GRAD_UPDATE_TIME / 1000000.0; /* sec */
  float pulseArea = (float) selrf->rf.flip * (PI / 180.0); /* radians or Hz (depending on how you look at it) */
  float subArea = (ks_waveform_absmax(rfLowRes, NPulses) * pulseArea) / (ks_waveform_sum(rfLowRes, NPulses) * 2.0 * PI * GAM); /* Gauss * sec */
  int hpw = ceil(subArea / (maxB1 * dt)); /* number of time points in sub pulse */
  float scaleFactor = 1.0;

  /* kludge :( */
  for (idx = 0; idx < NPulses; idx++) {
    if (areSame(rfLowRes[idx], 0)) {
      rfLowRes[idx] = rfLowRes[idx] + 0.01;
    }
  }

  /* Loop to concatenate the sub pulses and gradient blips */
  for (idx = 0; idx < NPulses; idx++) {
    for (jdx = 0; jdx < hpw; jdx++) {
      selrfPINS->rf.rfwave.waveform[kdx] = rfLowRes[idx] / scaleFactor;
      selrfPINS->gradwave.waveform[kdx] = 0.0;
      kdx++;
    }
    if (idx != NPulses - 1) {
      for (jdx = 0; jdx < gzBlipWave.res; jdx++) {
        selrfPINS->rf.rfwave.waveform[kdx] = 0.0;
        selrfPINS->gradwave.waveform[kdx] = gzBlipWave.waveform[jdx];
        kdx++; gdx++;
      }
    }
  }

  /* Set duration and resolution */
  selrfPINS->gradwave.gradwave_units = KS_GRADWAVE_ABSOLUTE;
  selrfPINS->gradwave.res = RUP_FACTOR(kdx, GRAD_UPDATE_TIME);
  selrfPINS->gradwave.duration = selrfPINS->gradwave.res * GRAD_UPDATE_TIME;
  selrfPINS->rf.rfwave.res = selrfPINS->gradwave.res;
  selrfPINS->rf.rfwave.duration = selrfPINS->gradwave.duration;
  selrfPINS->rf.rfpulse.isodelay = RUP_FACTOR(ceil((float)selrfPINS->rf.rfpulse.isodelay * ((float)selrfPINS->rf.rfwave.duration/(float)orgDur)), GRAD_UPDATE_TIME);
  selrfPINS->rf.iso2end = selrfPINS->rf.rfpulse.isodelay;
  selrfPINS->rf.bw = 1e3 * (NPulses+1) * selrf->slthick / sms_slice_gap;

  /* If the resolution has been rounded up, place zeros at the end */
  for (idx = kdx; idx < selrfPINS->rf.rfwave.res; idx++) {
    selrfPINS->rf.rfwave.waveform[idx] = 0.0;
    selrfPINS->gradwave.waveform[idx] = 0.0;
  }

  if (debug == 1) {ks_print_waveform(selrfPINS->rf.rfwave.waveform, "pins_RF.txt", selrfPINS->rf.rfwave.res); }
  if (debug == 1) {ks_print_waveform(selrfPINS->gradwave.waveform, "pins_GRAD.txt", selrfPINS->gradwave.res); }

  /* Normalize to 1 */
  ks_waveform_multiplyval(selrfPINS->rf.rfwave.waveform, 1.0 / ks_waveform_absmax(selrfPINS->rf.rfwave.waveform, selrfPINS->rf.rfwave.res), selrfPINS->rf.rfwave.res);

  /* rfStat */
  status = ks_eval_rfstat(&selrfPINS->rf); KS_RAISE(status);

  /* init theta wave */
  ks_init_wave(&selrfPINS->rf.thetawave);
  selrfPINS->rf.thetawave.duration = selrfPINS->gradwave.duration;
  selrfPINS->rf.thetawave.res = selrfPINS->gradwave.res;
  if (debug == 1) {ks_print_waveform(selrfPINS->rf.thetawave.waveform, "pins_TH.txt", selrfPINS->rf.thetawave.res); }

  /* Register the RF */
  KS_DESCRIPTION description;
  ks_create_suffixed_description(description,                     selrfPINS->rf.rfwave.description, "_PINS");
  ks_create_suffixed_description(selrfPINS->gradwave.description, selrfPINS->rf.rfwave.description, "_PINS");
  if (debug == 1) {fprintf(stderr, "---------- PINS %s rfstat ---------\n", selrfPINS->rf.rfwave.description);}
  if (debug == 1) {ks_print_rfpulse(selrfPINS->rf.rfpulse, stderr);}
  return ks_eval_rf(&selrfPINS->rf, description);

}




STATUS ks_eval_sms_make_pins_dante(KS_SELRF *selrfPINS, const KS_SELRF *selrf, float sms_slice_gap) {
  STATUS status;
  int jdx, idx, kdx = 0, gdx = 0, debug = 1;
  int orgDur = selrfPINS->rf.rfwave.duration;
  KS_TRAP gzblip = KS_INIT_TRAP;

  /* Duplicate the selRF structure  */
  if (selrfPINS != selrf ) {
    *selrfPINS = *selrf;
  }

  /* Set info struct */
  selrfPINS->sms_info.slice_gap = sms_slice_gap;
  selrfPINS->sms_info.pulse_type = KS_SELRF_SMS_PINS_DANTE;

  if (debug == 1) {ks_print_waveform(selrf->rf.rfwave.waveform, "dpins_rfMB_base.txt", selrf->rf.rfwave.res); }
  if (debug == 1) {ks_print_waveform(selrf->rf.thetawave.waveform, "dpins_phMB_base.txt", selrf->rf.thetawave.res); }

  /* Create blip */
  KS_WAVE gzBlipWave = KS_INIT_WAVE;
  double gArea = 1 / (sms_slice_gap / 10) / (GAM); /* (g sec)/cm, k-area of each blip */
  gzblip.area = gArea * 1e6; /* (G/cm)*us */
  status = ks_eval_trap1(&gzblip, "PINSblip"); KS_RAISE(status);
  status = ks_eval_trap2wave(&gzBlipWave, &gzblip); KS_RAISE(status);
  ks_init_trap(&gzblip);

  /* Calcualte PINS parameters */
  double kzw = (selrf->rf.bw * selrf->rf.rfwave.duration * 1e-6) / (selrf->slthick / 10); /* 1/cm, width in kz-space we must go */
  int NPulses = (floor(ceil(kzw / (1 / (sms_slice_gap / 10))) / 2) * 2) + 1; /* number of subpulses (odd) */
  int hpw = ceil((float)selrf->rf.rfwave.res / (float)NPulses); /* number of time points in sub pulse */
  int interpolRes = hpw * NPulses;

  /* Create index for interpolation */
  float *newGrid = (float*)alloca(interpolRes * sizeof(float));
  float *baseGrid = (float*)alloca(selrf->rf.rfwave.res * sizeof(float));
  for (idx = 0; idx < selrf->rf.rfwave.res; idx++) {
    baseGrid[idx] = (float) idx / (selrf->rf.rfwave.res - 1);
  }
  for (idx = 0; idx < interpolRes; idx++) {
    newGrid[idx] = (float) idx / (interpolRes - 1);
  }

  /* Interpolate the base rf & phase to make their resolution divideble by NPulses */
  KS_WAVEFORM rfNewRes = KS_INIT_WAVEFORM;
  KS_WAVEFORM phNewRes = KS_INIT_WAVEFORM;
  ks_eval_linear_interp1(baseGrid, selrf->rf.rfwave.res, selrf->rf.rfwave.waveform, newGrid, interpolRes, rfNewRes);
  ks_eval_linear_interp1(baseGrid, selrf->rf.thetawave.res, selrf->rf.thetawave.waveform, newGrid, interpolRes, phNewRes);

  if (debug == 1) {ks_print_waveform(rfNewRes, "rfNewRes.txt", interpolRes); }
  if (debug == 1) {ks_print_waveform(phNewRes, "phNewRes.txt", interpolRes); }

  /* Loop to concatenate the sub pulses and gradient blips */
  for (idx = 0; idx < NPulses; idx++) {
    for (jdx = 0; jdx < hpw; jdx++) {
      selrfPINS->rf.rfwave.waveform[kdx] = rfNewRes[(idx * hpw) + jdx];
      selrfPINS->rf.thetawave.waveform[kdx] = phNewRes[(idx * hpw) + jdx];
      selrfPINS->gradwave.waveform[kdx] = 0.0;
      kdx++;
    }
    if (idx != NPulses - 1) {
      for (jdx = 0; jdx < gzBlipWave.res; jdx++) {
        selrfPINS->rf.rfwave.waveform[kdx] = 0.0;
        selrfPINS->rf.thetawave.waveform[kdx] = 0.0;
        selrfPINS->gradwave.waveform[kdx] = gzBlipWave.waveform[jdx];
        kdx++; gdx++;
      }
    }
  }

  /* Set duration and resolution */
  selrfPINS->grad.duration = 0;
  selrfPINS->gradwave.gradwave_units = KS_GRADWAVE_ABSOLUTE;
  selrfPINS->gradwave.res = RUP_FACTOR(kdx, GRAD_UPDATE_TIME);
  selrfPINS->gradwave.duration = selrfPINS->gradwave.res * GRAD_UPDATE_TIME;
  selrfPINS->rf.rfwave.res = selrfPINS->gradwave.res;
  selrfPINS->rf.rfwave.duration = selrfPINS->gradwave.duration;
  selrfPINS->rf.rfpulse.isodelay = RUP_FACTOR((int)(selrfPINS->rf.rfpulse.isodelay * ((float)selrfPINS->rf.rfwave.duration/(float)orgDur)),RF_UPDATE_TIME);
  selrfPINS->rf.iso2end = selrfPINS->rf.rfpulse.isodelay;
  selrfPINS->rf.bw = 1e3 * (NPulses+1) * selrf->slthick / sms_slice_gap;

  /* If the resolution has been rounded up, place zeros at the end */
  for (idx = kdx; idx < selrfPINS->rf.rfwave.res; idx++) {
    selrfPINS->rf.rfwave.waveform[idx] = 0.0;
    selrfPINS->rf.thetawave.waveform[idx] = selrfPINS->rf.thetawave.waveform[kdx];
    selrfPINS->gradwave.waveform[idx] = 0.0;
  }

  if (debug == 1) {ks_print_waveform(selrfPINS->rf.rfwave.waveform, "dpins_RF.txt", selrfPINS->rf.rfwave.res); }
  if (debug == 1) {ks_print_waveform(selrfPINS->gradwave.waveform, "dpins_GRAD.txt", selrfPINS->gradwave.res); }

  /* Normalize to 1 */
  ks_waveform_multiplyval(selrfPINS->rf.rfwave.waveform, 1.0 / ks_waveform_absmax(selrfPINS->rf.rfwave.waveform, selrfPINS->rf.rfwave.res), selrfPINS->rf.rfwave.res);

  /* rfStat */
  float orgMaxB1 = selrfPINS->rf.rfpulse.max_b1;
  selrfPINS->rf.thetawave.res = 0;
  status = ks_eval_rfstat(&selrfPINS->rf); KS_RAISE(status);
  selrfPINS->rf.rfpulse.max_b1 = orgMaxB1;

  /* Theta wave */
  selrfPINS->rf.thetawave.duration = selrfPINS->gradwave.duration;
  selrfPINS->rf.thetawave.res = selrfPINS->gradwave.res;

  for (idx = 0; idx < selrfPINS->rf.thetawave.res; idx++) {
    if (fabs(selrfPINS->rf.thetawave.waveform[idx]) > 180.0) {
      double angle = selrfPINS->rf.thetawave.waveform[idx];
      double revolutions = floor((angle + 180.0) / 360.0);
      selrfPINS->rf.thetawave.waveform[idx] = (float) (angle - revolutions * 360.0);
    }
  }

  if (debug == 1) {ks_print_waveform(selrfPINS->rf.thetawave.waveform, "dpins_TH.txt", selrfPINS->rf.thetawave.res); }

  /* Register the RF */
  KS_DESCRIPTION description;
  ks_create_suffixed_description(description,                     selrfPINS->rf.rfwave.description, "_PINSDANTE");
  ks_create_suffixed_description(selrfPINS->gradwave.description, selrfPINS->rf.rfwave.description, "_PINSDANTE");
  if (debug == 1) {fprintf(stderr, "---------- PINS DANTE %s rfstat ---------\n", selrfPINS->rf.rfwave.description);}
  if (debug == 1) {ks_print_rfpulse(selrfPINS->rf.rfpulse, stderr);}
  return ks_eval_rf(&selrfPINS->rf, description);

}




int ks_eval_findNearestNeighbourIndex(float value, const float *x, int length) {

  float dist = fabs(value - x[0]);
  float newDist;
  int idx = 0, i;

  for (i = 1; i < length; i++) {

    newDist = value - x[i];

    if (0 < newDist && newDist < dist) {

      dist = newDist;
      idx = i;
    }
  }

  return idx;
}




void ks_eval_linear_interp1(const float *x, int x_length, const float *y, const float *xx, int xx_length, float *yy) {

  int i, index;
  float dx, dy;
  float *slope = (float*)alloca(x_length * sizeof(float));
  float *intercept = (float*)alloca(x_length * sizeof(float));
  float *Y = (float*)alloca(x_length * sizeof(float));
  memcpy(Y, y, x_length * sizeof(float));

  for (i = 0; i < x_length; i++) {

    if (i < x_length - 1) {

      dx = x[i + 1] - x[i];
      dy = Y[i + 1] - Y[i];
      slope[i] = dy / dx;
      intercept[i] = Y[i] - x[i] * slope[i];

    } else {

      slope[i] = slope[i - 1];
      intercept[i] = intercept[i - 1];

    }
  }

  for (i = 0; i < xx_length; i++) {

    index = ks_eval_findNearestNeighbourIndex(xx[i], x, x_length);
    yy[i] = slope[index] * xx[i] + intercept[index];
  }
} /* EOF */


STATUS ks_eval_readtrap2readwave(KS_READTRAP * readtrap, KS_READWAVE * readwave) {
  STATUS status;

  float t[4] = {0};
  float G[4] = {0};

  t[0] = 0;                                 G[0] = 0;
  t[1] = readtrap->grad.ramptime;           G[1] = readtrap->grad.amp;
  t[2] = t[1] + readtrap->grad.plateautime; G[2] = G[1];
  t[3] = t[2] + t[1];                       G[3] = 0;

  KS_WAVE gradwave = KS_INIT_WAVE;
  status = ks_eval_coords2wave(&gradwave, t, G, 4, GRAD_UPDATE_TIME, "monkeypoo");

  return status;


}




STATUS ks_eval_trap2wave(KS_WAVE *wave, const KS_TRAP *trap) {

  int i;
  int idx = 0;

  ks_init_wave(wave);

  int pointsInRamp = (trap->ramptime / GRAD_UPDATE_TIME) + 1;
  int pointsInPlateau = (trap->plateautime / GRAD_UPDATE_TIME) - 2;
  int pointsInWave = 2 * pointsInRamp + pointsInPlateau;
  double slope = trap->amp / pointsInRamp;

  if (trap->duration != (pointsInWave * GRAD_UPDATE_TIME)) {
    return ks_error("ks_eval_trap2wave(%s): field .ramptime or .plateautime not divisible by GRAD_UPDATE_TIME (4)", trap->description);
  }

  /* ramp up */
  for (i = 1; i <= pointsInRamp; i++) {
    wave->waveform[idx++] = (float)(i * slope);
  }

  /* plateau */
  for (i = 0; i < pointsInPlateau; i++) {
    wave->waveform[idx++] = trap->amp;
  }

  /* ramp down */
  for (i = pointsInRamp; i > 0; i--) {
    wave->waveform[idx++] = (float)(i * slope);
  }

  /* set fields */
  wave->res = idx;
  if (pointsInWave != wave->res) {
    return ks_error("ks_eval_trap2wave(%s): Implementation error - The resulting KS_WAVE has wrong res [%d!=%d]", trap->description, pointsInWave, wave->res);
  }

  wave->duration = idx * GRAD_UPDATE_TIME;
  if (trap->duration != wave->duration) {
    return ks_error("ks_eval_trap2wave(%s): Implementation error - The resulting KS_WAVE has wrong duration [%d!=%d]", trap->description,trap->duration,wave->duration);
  }

  ks_create_suffixed_description(wave->description, trap->description, "_wave");

  return SUCCESS;

} /* EOF */


STATUS ks_eval_coords2wave(KS_WAVE* wave, float* t, float* G, int num_coords, int dwell, const char* desc) {
  int i, bin;

  if (num_coords < 2) {
    return KS_THROW("%s: At least two coords must be provided, not %d", desc, num_coords);
  }
  if (fabs(t[0]) > 0.0f) {
    return KS_THROW("%s: First coord must be t=0, not %f", desc, t[0]);
  }

  for (i = 1; i < num_coords; i++) {
    if (t[i] < t[i-1]) {
      return KS_THROW("%s: Coords must be in time order (%f < %f)", desc, t[i], t[i-1]);
    }
  }

  int duration = (int)(t[num_coords-1]);
  if ((duration % (dwell)) || (fabs(t[num_coords-1] - (float)(duration)) > 0)) {
    return KS_THROW("%s: Last coord must be a multiple of %d, not %f", desc, dwell, t[num_coords-1]);
  }
  int res = duration / dwell;

  KS_WAVEFORM waveform = KS_INIT_WAVEFORM;

  /* loop over line segments */
  for (i = 1; i < num_coords; i++) {
    if ((t[i] - t[i-1]) <= 0) {
      continue; /* empty segment */
    }
    float slope = (G[i] - G[i-1]) / (t[i] - t[i-1]);
    int startbin = (int) floor(t[i-1] / dwell);
    int endbin = (int) floor(t[i] / dwell);

    /* startbin */
    float dur = FMin(2, t[i], (float)((startbin+1) * dwell)) - t[i-1];
    waveform[startbin] += dur * (G[i-1] + dur * slope / 2.0) / dwell;

    /* middle bins */
    for (bin = startbin + 1; bin < endbin; bin++) {
      waveform[bin] = G[i-1] + (dwell * ((float)(bin) + 0.5) - t[i-1]) * slope;
    }

    /* endbin */
    if ((startbin != endbin) && (endbin < res)) {
      dur = t[i] - dwell * (float)(endbin);
      waveform[endbin] += dur * (G[i] - dur * slope / 2.0) / dwell;
    }
  }

  return ks_eval_wave(wave, desc, res, duration, waveform);
}




STATUS ks_eval_append_two_waves(KS_WAVE* first_wave, KS_WAVE* second_wave) {
  STATUS status;
  /* check inputs */
  if (first_wave->res > 0 && second_wave->res > 0 &&
      (first_wave->duration / first_wave->res) != (second_wave->duration / second_wave->res)) {
    return ks_error("%s: can't append waves with different dwell periods.", __FUNCTION__);
  }

  status = ks_eval_append_two_waveforms(first_wave->waveform, second_wave->waveform, first_wave->res, second_wave->res);
  KS_RAISE(status);

  /* fill in params */
  first_wave->res += second_wave->res;
  first_wave->duration += second_wave->duration;
  first_wave->area += second_wave->area;
  if (second_wave->max_amp > first_wave->max_amp) {
    first_wave->max_amp = second_wave->max_amp;
  }
  if (second_wave->abs_max_amp > first_wave->abs_max_amp) {
    first_wave->abs_max_amp = second_wave->abs_max_amp;
  }
  if (second_wave->abs_max_slew > first_wave->abs_max_slew) {
    first_wave->abs_max_slew = second_wave->abs_max_slew;
  }
  if (second_wave->min_amp < first_wave->min_amp) {
    first_wave->min_amp = second_wave->min_amp;
  }

  return SUCCESS;

}




STATUS ks_eval_append_two_waveforms(KS_WAVEFORM first_waveform, KS_WAVEFORM second_waveform, int res1, int res2) {
  if ((res1 + res2) > KS_MAXWAVELEN) {
    return ks_error("%s: cannot append - the combined waveform exceeds KS_MAXWAVELEN.", __FUNCTION__);
  }
  memcpy( &(first_waveform[res1]), second_waveform, sizeof(float) * res2);
  return SUCCESS;
}




STATUS ks_eval_concatenate_waves(int num_waves, KS_WAVE* target_wave, KS_WAVE** waves_to_append) {
  int idx;
  STATUS status;

  for(idx = 0; idx < num_waves; idx++) {
    status = ks_eval_append_two_waves(target_wave, waves_to_append[idx]);
    KS_RAISE(status);
  }

  return SUCCESS;
}




STATUS ks_eval_epi_constrained(KS_EPI *epi, const char * const desc, float ampmax, float slewrate) {

  STATUS status;
  char tmpstr[1000];
  int kspacelines_noacc;
  /* same code as ks_eval_dixon_dualreadtrap  */
  const float ampmax_phys = FMin(3, phygrd.xfs,    phygrd.yfs,    phygrd.zfs);
  const int ramptimemax_phys = IMax(3, phygrd.xrt, phygrd.yrt, phygrd.zrt);
  const float slewrate_phys = ampmax_phys / ramptimemax_phys;
  float slewrate_phasers; /* lower slew rate for phasers before/after the train */
  float ampmax_phasers; /* lower ampmax for phasers before/after the train */

  /* for now spherical model such that out of spec slewrate or amplitude cannot happen (for the phasers), even with prospective moco */
  if (epi->zphaser.res > 1) {
    slewrate_phasers = FMin(2, slewrate, slewrate_phys / sqrt(3));
    ampmax_phasers = FMin(2, ampmax, ampmax_phys / sqrt(3));
  } else {
    slewrate_phasers = FMin(2, slewrate, slewrate_phys / sqrt(2));
    ampmax_phasers = FMin(2, ampmax, ampmax_phys / sqrt(2));
  }

  if (desc == NULL || desc[0] == ' ') {
    return ks_error("%s: description (2nd arg) cannot be NULL or begin with a space", __FUNCTION__);
  }

  /************************** Step 1: resolution checking ************************/

  /* For EPI, we don't allow negative 'nover'. For lower k-space pFourier control, we use ks_phaseencoding_generate_epi() */
  if (epi->blipphaser.nover < 0) {
    return ks_error("%s(%s): negative 'blipphaser.nover' not allowed for EPI. See ks_phaseencoding_generate_epi()", __FUNCTION__, desc);
  }
  if (abs(epi->read.nover) > 0) {
    return ks_error("%s(%s): partial Fourier in read is not supported", __FUNCTION__, desc);
  }
  if (epi->minbliparea < 0 || epi->minbliparea > 1000) {
    return ks_error("%s(%s): field 'minbliparea' must be in range [0,1000]", __FUNCTION__, desc);
  }
  if (epi->read.res % 2) {
    return ks_error("%s(%s): Read res must be even", __FUNCTION__, desc);
  }

  /************************** Step 2: blip phaser (DE/REphasers)  ***************************/
  epi->blipphaser.nacslines = 0; /* ACS lines are forbidden for EPI */
  epi->blipphaser.shotresalign = TRUE; /* make .res is divisible by 2*R in ks_eval_phaser_adjustres() */
  /* blip DE/REphaser (KS_PHASER) */
  sprintf(tmpstr, "%s.blipphaser", desc);
  status = ks_eval_phaser_constrained(&epi->blipphaser, tmpstr, ampmax_phasers, slewrate_phasers, 0);
  KS_RAISE(status);

  /************************** Step 3: z phaser ***************************/
  epi->blipphaser.shotresalign = FALSE; /* do not force .res to be divisible by 2*R in ks_eval_phaser_adjustres() */
  /* blip DE/REphaser (KS_PHASER) */
  sprintf(tmpstr, "%s.zphaser", desc);
  if ((epi->blipphaser.nover > 0) && (epi->zphaser.nover > 0)) {
    return ks_error("%s: Cannot have partial Fourier in both ky (nover=%d) and kz (nover=%d)", __FUNCTION__, epi->blipphaser.nover, epi->zphaser.nover);
  }
  if (epi->zphaser.res > 1) {
    status = ks_eval_phaser_constrained(&epi->zphaser, tmpstr, ampmax_phasers, slewrate_phasers, 0);
    KS_RAISE(status);
  } else {
    ks_init_phaser(&epi->zphaser);
  }

  /************************** Step 4: ETL ***************************/

  if (epi->blipphaser.nover > 0) {
    kspacelines_noacc = epi->blipphaser.res / 2 + epi->blipphaser.nover;
  } else {
    kspacelines_noacc = epi->blipphaser.res;
  }

  epi->etl = kspacelines_noacc / epi->blipphaser.R;

  if (epi->etl < 1) {
    return ks_error("%s(%s): R (or shots) must be in range [1,%d]", __FUNCTION__, desc, kspacelines_noacc);
  }

  /********************* Step 5: EPI blips ********************/

  /* blip area [(G/cm)*usec] */
  if (epi->etl > 1) {

    epi->blip.area = epi->blipphaser.R * ks_calc_fov2gradareapixel(epi->blipphaser.fov);

    /* The blip area should not be too small to reduce discretization errors. This is based on observations doing moment calcs in WTools.  */
    if (epi->blip.area < epi->minbliparea)
      epi->blipoversize = (epi->minbliparea / epi->blip.area); /* design a bigger blip, and use .blipoversize in ks_scan_epi_shotcontrol() to reduce the amp correspondingly */
    else
      epi->blipoversize = 1.0;

    epi->blip.area = epi->blipphaser.R * ks_calc_fov2gradareapixel(epi->blipphaser.fov) * epi->blipoversize;
  } else {
    epi->blip.area = 0.0;
  }
  /* Get ramp & plateau time as well as amplitude of the blip */
  sprintf(tmpstr, "%s.blip", desc);

  status = ks_eval_trap_constrained(&epi->blip, tmpstr, ampmax, slewrate, 0);
  KS_RAISE(status);

  /************************** Step 6: CAIPI-blips **************************/
  if (epi->etl > 1) {
    sprintf(tmpstr, "%s.caipiblip", desc);
    status = ks_eval_trap_constrained(&epi->caipiblip, tmpstr, ampmax, slewrate, 0);
    KS_RAISE(status);
  }

  /************************** Step 7: Read width and amp **************************/

  /* rampsampling (normal case): We don't want to acquire data during half the blips duration (straddling two readouts) */
  int half_max_blip_dur = IMax(2, epi->blip.duration, epi->caipiblip.duration) / 2;
  if (epi->read.rampsampling)
    epi->read.acqdelay = half_max_blip_dur;

  sprintf(tmpstr, "%s.read", desc);
  status = ks_eval_readtrap_constrained(&epi->read, tmpstr, ampmax, slewrate);
  KS_RAISE(status);

  if (!epi->read.rampsampling) {
    /* non-rampsampling (unusual case) */
    if (epi->read.grad.ramptime < half_max_blip_dur) {
      /* blip-duration limited: Prolong the read gradient ramps to fit the blip, reduce PNS and acoustic noise */
      epi->read.grad.ramptime = half_max_blip_dur;
      epi->read.acqdelay = epi->read.grad.ramptime;
      epi->read.grad.duration = epi->read.grad.plateautime + 2 * epi->read.grad.ramptime;
      epi->read.grad.area = (epi->read.grad.plateautime + epi->read.grad.ramptime) * epi->read.grad.amp;
      epi->read.area2center = epi->read.grad.area / 2;
    }
  }

  if (epi->epi_readout_mode == KS_EPI_FLYBACK) {
    sprintf(tmpstr, "%s.read_flyback", desc);
    epi->read_flyback.area = epi->read.grad.area;
    if (ks_eval_trap_constrained(&epi->read_flyback, desc, ampmax, slewrate, 0) == FAILURE){
      return FAILURE;
    };
  }

  /************************** Step 8: read phaser (DE/REphasers) **************************/

  /* readphaser (KS_TRAP) */
  epi->readphaser.area = -epi->read.area2center;

  if (!areSame(epi->readphaser.area, 0.0)) {
    sprintf(tmpstr, "%s.readphaser", desc);
    status = ks_eval_trap_constrained(&epi->readphaser, tmpstr, ampmax_phasers, slewrate_phasers, 0);
    KS_RAISE(status);
  }



  /**************************** Step 9: Convenience info ****************************/

  ks_eval_epi_setinfo(epi);


  return SUCCESS;

} /* ks_eval_epi_constrained */



STATUS ks_eval_epi_setinfo(KS_EPI *epi) {
  const int readfact = (epi->epi_readout_mode == KS_EPI_SPLITODDEVEN) ? 2 : 1;

  epi->duration = (epi->etl * readfact) * epi->read.grad.duration + \
                  ((epi->etl * readfact) - 1) * (epi->read_spacing + epi->read_flyback.duration) + \
                  IMax(3, epi->readphaser.duration, epi->blipphaser.grad.duration, epi->zphaser.grad.duration) + \
                  IMax(3, epi->readphaser.duration, epi->blipphaser.grad.duration, epi->zphaser.grad.duration);

  if (abs(epi->blipphaser.nover)) {
    /* partial Fourier */
    epi->time2center = ((epi->read.grad.duration + epi->read_flyback.duration + epi->read_spacing) * readfact * (epi->blipphaser.nover / epi->blipphaser.R)) - (epi->read_spacing + epi->read_flyback.duration) / 2;
  } else {
    epi->time2center = ((epi->read.grad.duration + epi->read_flyback.duration + epi->read_spacing) * readfact * (epi->etl / 2)) - (epi->read_spacing + epi->read_flyback.duration) / 2;
  }
  epi->time2center += IMax(3, epi->readphaser.duration, epi->blipphaser.grad.duration, epi->zphaser.grad.duration);

  return SUCCESS;  
}




STATUS ks_eval_epi_maxamp_slewrate(float *ampmax, float *slewrate, int xres, float quietnessfactor) {
  PHYS_GRAD epiphygrd = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  LOG_GRAD  epiloggrd = {0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int epi_initnewgeo = 1;
  extern SCAN_INFO scan_info[SLTAB_MAX];
  extern int opcoax;
  extern int opplane;
  extern int opslquant;
  extern int obl_debug;
  int stronggradient_flag = 0;
  int premiersystem_flag = 0;
  const float premierslewrate_max = 0.0150; /* 0.0150 (G/cm)/us = SR150 (T/m)/s */
  const float stronggradient_ampmax = 3.5; /* 35 mT/m. Maximum gradient amplitude to avoid too high rBW at k-space center */

  /* relationships below have been tested in the range [16,256] */
  if (xres < 16)
    xres = 16;
  if (xres > 256)
    xres = 256;


  if (quietnessfactor < 1.0) {
    return ks_error("%s: The quietness factor must be >= 1.0", __FUNCTION__);
  }


  inittargets(&epiloggrd, &epiphygrd);

  /* we need to know the system limits, but we don't want to be dependent on GERequired:GEReq_init_gradspecs() and ks_srfact
     for the EPI readout. I.e. ks_srfact (GERequired.e) can be used to set a proper slewrate tweak factor for all gradients except for
     the EPI readout (and EPI blips), while the ampmax and slewrate for the EPI train is controlled only by this function */
  if (obloptimize_epi(&epiloggrd, &epiphygrd, scan_info, (opslquant),
                      (opplane), (opcoax), PSD_OBL_OPTIMAL, (obl_debug), &epi_initnewgeo, cfsrmode) == FAILURE) {
    return ks_error("%s: obloptimize_epi() failed", __FUNCTION__);
  }

  /* ampmax */
#if EPIC_RELEASE >= 27
  stronggradient_flag = (cfgcoiltype == PSD_XRMB_COIL || cfgcoiltype == PSD_HRMW_COIL); /* 750 or Premier. 50+ mT/m */
  premiersystem_flag = (cfgcoiltype == PSD_HRMW_COIL);
#else
  stronggradient_flag = cfgcoiltype == PSD_XRMB_COIL; /* 750 */
#endif

  if (stronggradient_flag) {
   *ampmax = FMin(2, stronggradient_ampmax, 0.01 * xres + 2); /* very unscientific linear relationship, but GE's EPI gradients increase semi-linearly with xres (they use epigradopt.c) */
  } else {
    /* 750w and others */
    if (xres > 128)
      *ampmax = 0.005 * xres + 1.36; /* allow the amp to increase slowly for higher res than 128 */
    else
      *ampmax = 2.0;
  }
  *ampmax = FMin(2, *ampmax, epiphygrd.xfs);


  /* slewrate */
  if (stronggradient_flag) {
    /* 750 or Premier. 50+ mT/m */
    if (xres <= 48){
      *slewrate = 250e-4; /* SR250 */
    } else {
      *slewrate = 1.0e-4 / (2.4e-5 * xres + 0.003); /* SR250 (xres 48) -> SR110 (xres 256). 1/x function */
    }
    if (premiersystem_flag && *slewrate > premierslewrate_max) {
      *slewrate = premierslewrate_max;
    }
  } else {
    /* 750w. 33 mT/m, SR120 (and all others) */
    if (xres <= 48)
      *slewrate = 170e-4; /* SR170 */
    else
      *slewrate = 120e-4; /* SR120 */

    /* use the relative ampmax reduction dependent on slice angulation to instead reduce the slewrate on 750w.
       ks_syslimits_ampmax2(&epiloggrd) returns 1.0 for axial scans,
       but for oblique scans the value is reduced. Although this should cap the amplitude, we use
       is here to instead reduce the slewrate, since we need some way to make the slewrate lower for (double)
       oblique scans, or maybe when the XGRAD is not parallel to the physical X-axis (R/L) */

    *slewrate *= epiloggrd.tx_xy / epiloggrd.xfs;
  }

  /* but if user wants a quieter scan, reduce the slewate */
  *slewrate /= quietnessfactor;


  return SUCCESS;

} /* ks_eval_epi_maxamp_slewrate() */



STATUS ks_eval_epi(KS_EPI *epi, const char * const desc, float quietnessfactor) {

  STATUS status;
  float ampmax, slewrate;

  status = ks_eval_epi_maxamp_slewrate(&ampmax, &slewrate, epi->read.res, quietnessfactor);
  KS_RAISE(status);

  return ks_eval_epi_constrained(epi, desc, ampmax, slewrate);

} /* ks_eval_epi */



STATUS ks_eval_echotrain(KS_ECHOTRAIN * const echotrain) {
  int readout = 0;

  echotrain->numtraps = -1;
  echotrain->numwaves = -1;

  int kspaces[KS_MAX_NUM_KSPACES];
  int kspace_encodes_per_shot[KS_MAX_NUM_KSPACES];

  int s;
  int i;
  echotrain->numstates = 0;
  for (s=0; s < KS_READCONTROL_MAXSTATE; s++){
    if (echotrain->controls[0].state[s].kspace_index == KS_NOTSET) {
      break;
    } else {
      echotrain->numstates++;
    }
  }

  for (s = 0; s < echotrain->numstates; s++) {

    echotrain->numkspaces[s] = 0;
    for (i = 0; i < KS_MAX_NUM_KSPACES; i++) {
      kspaces[i] = 0;
      kspace_encodes_per_shot[i] = 0;
    }

    for (readout = 0; readout < KS_MAXUNIQUE_READ; readout++) { /* readouts */

      /* We assume that the first readout with a uninitialized type marks the end of the array */
      if (echotrain->controls[readout].pg.read_type == KS_READ_NOTSET) {
        break;
      }

      KS_READCONTROL* control = &echotrain->controls[readout];
      KS_READ_TYPE type = control->pg.read_type;
      int object_index = control->pg.object_index;
      if (type == KS_READ_TRAP) {
        if (object_index > echotrain->numtraps) {
          echotrain->numtraps = object_index;
        }
      } else if (type == KS_READ_WAVE) {
        if (control->state[s].wave_state == KS_NOTSET) {
          return KS_THROW("Readout %d shot state %d has no wave_state. You probably forgot to set it", readout, s);
        }
        if (object_index > echotrain->numwaves) {
          echotrain->numwaves = object_index;
        }
        /*TODO if (echotrain->readwaves[object_index].grad.base.nstates != echotrain->numstates) {
          return ks_error("echotrain->readwaves[%i]: mismatch in wavestates [echotrain: %i != readwave: %i]", object_index, echotrain->numstates, echotrain->readwaves[object_index].grad.base.nstates);
        }*/
      }

      KS_READCONTROL_STATE * state = &control->state[s];
      /* Attempt finding kspace_index in the kspaces array */
      for(i = 0; i < echotrain->numkspaces[s]; i++) {
        if(kspaces[i] == state->kspace_index) {
          break;
        }
      }
      if (i == echotrain->numkspaces[s]) { 
        /* not found: append at the end */
        kspaces[i] = state->kspace_index;
        echotrain->numkspaces[s]++;
        if (echotrain->numkspaces[s] > KS_MAX_NUM_KSPACES) {
          return KS_THROW("Echotrain supports up to %d unique kspaces.", KS_MAX_NUM_KSPACES);
        }
      }
      /* increase the count of how many times each kspace_index occurs in the echotrain */
      kspace_encodes_per_shot[kspaces[i]]++;

    } /* readout loop */

  if (echotrain->numkspaces[s] > echotrain->numkspaces[0]) {
    return KS_THROW("Variable numkspaces per readstate not supported -- state[0] = %i : state[%i] = %i.", echotrain->numkspaces[0], s, echotrain->numkspaces[s]);
  }
  for (i = 0; i < echotrain->numkspaces[s]; i++) {
    if(kspace_encodes_per_shot[kspaces[i]] != kspace_encodes_per_shot[kspaces[0]]) {
      return KS_THROW("Variable encodes per shot for kspaces is not supported (%d != %d)", kspace_encodes_per_shot[kspaces[i]], kspace_encodes_per_shot[kspaces[0]]);
    }
  }

  } /* state loop */

  echotrain->numtraps++;
  echotrain->numwaves++;
  echotrain->numreadouts = readout;

  return SUCCESS;
} /* ks_eval_echotrain */


GRAD_PULSE ks_eval_makegradpulse(KS_TRAP *trp, int gradchoice) {
  GRAD_PULSE g;

  /* we have to compute it here because g.amp is a pointer */
  trp->heat_scaled_amp[gradchoice] = trp->amp*trp->heat_scaling_factor[gradchoice];


  g.ptype = G_TRAP;
  g.attack = &trp->ramptime;
  g.decay = &trp->ramptime;
  g.pw = &trp->plateautime;
  g.amps = NULL;
  g.amp = &trp->heat_scaled_amp[gradchoice];
  g.ampd = NULL;
  g.ampe = NULL;
  g.gradfile = NULL;
  g.num = trp->gradnum[gradchoice];
  g.scale = 1.0; /* scale. < 1 for phase enc. */
  g.time = NULL;
  g.tdelta = 0; /* Time delta in microseconds in between multiple occurances of the pulse */
  g.powscale = 1.0; /* might want to do loggrd.xfs/loggrd.tx_xyz instead? */

  /* the following are set by minseq***() later */
  g.power = 0.0;
  g.powpos = 0.0;
  g.powneg = 0.0;
  g.powabs = 0.0;
  g.amptran = 0.0;
  g.pwm = 1;
  g.bridge = 0;
  g.intabspwmcurr = 0;

  return g;
}




STATUS ks_eval_seqctrl_setminduration(KS_SEQ_CONTROL *seqctrl, int mindur) {
#ifndef IPG
  /* only do this on HOST */

  if (seqctrl == NULL) {
    return ks_error("%s: arg 1 is NULL", __FUNCTION__);
  } else if (mindur > 0 && seqctrl->ssi_time <= 0) {
    return ks_error("%s: %s - seqctrl.ssi_time must be > 0 before setting minimum duration", __FUNCTION__, seqctrl->description);
  } else if (mindur > 0 && seqctrl->ssi_time % 4) {
    return ks_error("%s: %s - seqctrl.ssi_time must be divisible by GRAD_UPDATE_TIME", __FUNCTION__, seqctrl->description);
  } else if (mindur < 0) {
    return ks_error("%s: %s - min duration (arg 2) must be >= 0", __FUNCTION__, seqctrl->description);
  } else if (mindur % 4) {
    return ks_error("%s: %s - min duration (arg 2) must be divisible by GRAD_UPDATE_TIME", __FUNCTION__, seqctrl->description);
  } else {
    if (mindur == 0) {
      seqctrl->min_duration = 0;
    } else {
      /* we add 4us (GRAD_UPDATE_TIME) to avoid waveform-after-seqcore errors */
      seqctrl->min_duration = RUP_GRD(mindur + seqctrl->ssi_time + GRAD_UPDATE_TIME);
    }
    seqctrl->duration = seqctrl->min_duration;
  }
#endif

 return SUCCESS;
}




STATUS ks_eval_seqctrl_setduration(KS_SEQ_CONTROL *seqctrl, int dur) {
#ifndef IPG
  /* only do this on HOST */

  if (seqctrl == NULL) {
    return ks_error("%s: arg 1 is NULL", __FUNCTION__);
  } else if (dur < seqctrl->min_duration) {
    return ks_error("%s: duration (arg 2) must be >= min duration (%d", __FUNCTION__, seqctrl->min_duration);
  } else {
    seqctrl->duration = RUP_GRD(dur);
  }
#endif

 return SUCCESS;
}




STATUS ks_eval_seqcollection_durations_setminimum(KS_SEQ_COLLECTION *seqcollection) {
#ifndef IPG
  /* only do this on HOST */

 int i;
  if (seqcollection != NULL) {
    for (i = 0; i < seqcollection->numseq; i++) {
      if (seqcollection->seqctrlptr[i]->min_duration % 4) {
        return ks_error("%s: Field min_duration for sequence module #%d is not divisible by GRAD_UPDATE_TIME", __FUNCTION__, i);
      }
      seqcollection->seqctrlptr[i]->duration = seqcollection->seqctrlptr[i]->min_duration;
    }
  }
#endif

  return SUCCESS;
}




STATUS ks_eval_seqcollection_durations_atleastminimum(KS_SEQ_COLLECTION *seqcollection) {
#ifndef IPG
  /* only do this on HOST */

 int i;
  if (seqcollection != NULL) {
    for (i = 0; i < seqcollection->numseq; i++) {
      if (seqcollection->seqctrlptr[i]->min_duration % 4) {
        return ks_error("%s: Field min_duration for sequence module #%d is not divisible by GRAD_UPDATE_TIME", __FUNCTION__, i);
      }
      if (seqcollection->seqctrlptr[i]->duration < seqcollection->seqctrlptr[i]->min_duration) {
        seqcollection->seqctrlptr[i]->duration = seqcollection->seqctrlptr[i]->min_duration;
      }
    }
  }
#endif

  return SUCCESS;
}




struct _grad_heat_handle {
  KS_SEQ_COLLECTION *seqcollection_p;
  STATUS (*play)(const INT max_encode_mode);
};

/*  global handle for gradient heating */
struct _grad_heat_handle grad_heat_handle = {NULL, NULL};

void ks_grad_heat_init(KS_SEQ_COLLECTION *seqcollection_p,
                       STATUS (*play)(const INT max_encode_mode)) {
  grad_heat_handle.seqcollection_p = seqcollection_p;
  grad_heat_handle.play = play;
}




void ks_grad_heat_reset() {
  grad_heat_handle.seqcollection_p = NULL;
  grad_heat_handle.play = NULL;
}




struct _grad_list {
  t_list *data[3];
  int buffer_sizes[3];
  int num_grads[3];
  int max_time;
};

STATUS _grad_list_init(struct _grad_list *grads) {
  int i;

  grads->max_time = 0;

  for (i=0; i<3; ++i) {
    grads->buffer_sizes[i] = 1024;
    grads->num_grads[i] = 1;

    t_list *p = (t_list*)malloc(grads->buffer_sizes[i]*sizeof(t_list));
    if (!p) {
      return KS_THROW("failed allocation");
    }
    grads->data[i] = p;
    grads->data[i][0].time = 0;
    grads->data[i][0].ampl = 0.0f;
    grads->data[i][0].ptype = G_USER;
  }
  return SUCCESS;
}




STATUS _grad_list_append(struct _grad_list *grads, t_list *grad, int board) {
  if (board < 0 || board > 2) {
    /* Board index out of bounds. Skipping... */
    return SUCCESS;
  }

  if (grads->num_grads[board] >= grads->buffer_sizes[board]) {
    /*  resize the buffer by 50%
     add 4 on top just in case the size starts at zero or close to it */
    grads->buffer_sizes[board] = (1.5*grads->num_grads[board]) + 4;
    t_list *p = (t_list*)realloc(grads->data[board], grads->buffer_sizes[board]*sizeof(t_list)); 
    if (!p) {
      return KS_THROW("failed reallocation");
    }
    grads->data[board] = p;
  }

  grads->data[board][grads->num_grads[board]] = *grad;
  grads->num_grads[board]++;

  return SUCCESS;
}




int _grad_compare(const void *_a, const void *_b) {

  t_list *a = (t_list*)_a;
  t_list *b = (t_list*)_b;

  return a->time - b->time;
}




STATUS _grad_list_remove_duplicates(struct _grad_list *grads) {
  int b, i, j;
  const char board[] = "XYZ";

  for (b=0; b<3; ++b) {

    if (grads->num_grads[b] <= 1) {
      continue;
    }

    /* sort */
    qsort(grads->data[b], grads->num_grads[b], sizeof(t_list), _grad_compare);


    for (i=0, j=1; j<grads->num_grads[b]; ++j) {
      if (grads->data[b][i].time > grads->data[b][j].time) {
        return KS_THROW("entries on board %c need to be time sorted", board[b]);
      }

      if (grads->data[b][i].time < grads->data[b][j].time) {
        /* the entry at j needs to be kept, update i to the new position for this entry */
        ++i;
        /* copy the entry if necessary */
        if (i != j) {
          grads->data[b][i] = grads->data[b][j];
        }
        continue;
      }

      /* equal time */
      /* corner points marked with ptype G_CONSTANTS are extra zeros and
         may be dropped even if the amplitude does not match */
      if (grads->data[b][j].ptype != G_CONSTANT &&
          fabs(grads->data[b][i].ampl - grads->data[b][j].ampl) > /* tolerance */ 0.01 *
          (fabs(grads->data[b][i].ampl) + fabs(grads->data[b][i].ampl))) {

        if (grads->data[b][i].ptype == G_CONSTANT) {
          /* Swap out the i-th entry for the j-th */
          grads->data[b][i] = grads->data[b][j];
        } else {
          /* Neither of the entries has ptype G_CONSTANT */
          return KS_THROW("amplitude differ (%.4f != %.4f) for the same time point (%d) on board %c", grads->data[b][i].ampl, grads->data[b][j].ampl, grads->data[b][i].time, board[b]);
        }
      }

      /* nothing to do... the entry at j will be "discarded" */
    }

    grads->num_grads[b] = i+1;
  }

  return SUCCESS;
}




STATUS _grad_list_add_sequence(struct _grad_list *grads, KS_SEQ_CONTROL *ctrl) {
  int i, j, k;
  STATUS s;

  if (!ctrl || !grads) {
    return KS_THROW("NULL inputs");
  }

  /* Add trapezoids */
  for (i = 0; i < ctrl->gradrf.numtrap; ++i) { /* each unique trap object */
    const KS_TRAP* trap = ctrl->gradrf.trapptr[i];

    for (j = 0; j < trap->base.ninst; ++j) { /* each instance of the trap */
      KS_SEQLOC loc = trap->locs[j];
      const float rtscale = ks_rt_scale_log_get(&trap->rtscaling, j, ctrl->current_playout);

      /* add the starting corner */
      t_list grad = {grads->max_time + loc.pos, 0.0, G_USER};
      s = _grad_list_append(grads, &grad, loc.board);
      KS_RAISE(s);

      /* add the end of the ramp */
      grad.time += trap->ramptime;
      grad.ampl = trap->amp * loc.ampscale * rtscale;
      s = _grad_list_append(grads, &grad, loc.board);
      KS_RAISE(s);

      /* add the end of the plateau */
      grad.time += trap->plateautime;
      s = _grad_list_append(grads, &grad, loc.board);
      KS_RAISE(s);

      /* add the end of the trapezoid */
      grad.time += trap->ramptime;
      grad.ampl = 0.0;
      s = _grad_list_append(grads, &grad, loc.board);
      KS_RAISE(s);      
    }
  }

  /* Add waves */
  for (i = 0; i < ctrl->gradrf.numwave; ++i) { /* each unique wave object */
    const KS_WAVE* waveptr = ctrl->gradrf.waveptr[i];

    const int dwell = waveptr->duration / waveptr->res;
    if (dwell % GRAD_UPDATE_TIME) {
      return KS_THROW("dwell time must be a multiple of the gradient update time");
    }

    /* Extra corner points with zero amplitude are marked with ptype G_CONSTANT */
    for (j = 0; j < waveptr->base.ninst; ++j) { /* each instance of the wave */
      KS_SEQLOC loc = waveptr->locs[j];
      const float rtscale = ks_rt_scale_log_get(&waveptr->rtscaling, j, ctrl->current_playout);

      t_list grad = {grads->max_time + loc.pos, 0.0, G_CONSTANT};
      s = _grad_list_append(grads, &grad, loc.board);
      KS_RAISE(s);

      grad.time = grads->max_time + loc.pos + dwell/2; 
      grad.ptype = G_USER;
      for (k = 0; k < waveptr->res; ++k) {
        grad.ampl = waveptr->waveform[k] * loc.ampscale * rtscale;
        s = _grad_list_append(grads, &grad, loc.board);
        KS_RAISE(s);
        grad.time += dwell;
      }

      grad.time -= dwell/2;
      grad.ampl = 0.0;
      grad.ptype = G_CONSTANT;
      s = _grad_list_append(grads, &grad, loc.board);
      KS_RAISE(s);
    }
  }

  /* add a point for each board at the end of the module */
  t_list grad = {grads->max_time + ctrl->duration, 0.0, G_USER};
  for (i=0; i<3; ++i) {
    s = _grad_list_append(grads, &grad, i);
    KS_RAISE(s);
  }

  /* update the max_time */
  grads->max_time = grad.time;

  ctrl->current_playout++;

  return SUCCESS;
}




#if EPIC_RELEASE < 27
#define MSS_FLAGS_T void
#else
#define MSS_FLAGS_T mss_flags_t
#endif

/* from getCornerPoints.cpp */
static INT _fillcp( t_list *gradlist[3], INT *n, FLOAT *tout, FLOAT **aout,
            FLOAT **ptypout, INT *nout );
static STATUS _fillArrays( FLOAT *Ttmp, FLOAT *Atmp, FLOAT *Typtmp,
                          t_list *List, INT num_elements, DOUBLE Tmax,
                          INT *Maxn );

/* Function potentially overriding GE's getCornerPoints if
   -Wl,-wrap,getCornerPoints is added to the linker flags
   on host/sim. */
#ifdef __cplusplus
extern "C"
#endif

#if EPIC_RELEASE > 26
STATUS __wrap_getCornerPoints( FLOAT **p_time, /* us rounded to samp_rate */
                               FLOAT *ampl[3], /* G/cm, logical */
                               FLOAT *pul_type[3], /* RAMP or USER, probably not used anyway */
                               INT *num_totpoints, /* number of corner points */
                               INT *num_iters, /* number of core modules playout? */
                               const LOG_GRAD *log_grad, /* input, can be ignored */
                               const INT seq_entry_index, /* input, can be ignored */
                               const INT samp_rate, /* usually set to GRAD_UPDATE_TIME */
                               const INT min_tr, /* input, can be ignored */
                               const FLOAT dbdtinf, /* input, can be ignored */
                               const FLOAT dbdtfactor, /* input, can be ignored */
                               const FLOAT efflength, /* input, can be ignored */
                               const INT max_encode_mode, /* MAXIMUM_POWER, AVERAGE_POWER */
                               const MSS_FLAGS_T* flags) /* input, can be ignored */
#else
STATUS __wrap_getCornerPoints( FLOAT **p_time,
                               FLOAT *ampl[3],
                               FLOAT *pul_type[3],
                               INT *num_totpoints,
                               const LOG_GRAD *log_grad,
                               const INT seq_entry_index,
                               const INT samp_rate,
                               const INT min_tr,
                               const FLOAT dbdtinf,
                               const FLOAT dbdtfactor,
                               const FLOAT efflength,
                               const INT max_encode_mode,
                               const dbLevel_t debug )
#endif
{
  STATUS s;
  int i;

  /*ks_dbg("getCornerPoints mode: %s", max_encode_mode == MAXIMUM_POWER ? "MAXIMUM" : "AVERAGE");*/

  /* validate global */
  if (!grad_heat_handle.seqcollection_p || !grad_heat_handle.play) {
    return KS_THROW("the function ks_grad_heat_init must be called with valid inputs prior to running the gradient heating model");
  }

  /* using samp rate that is different from  */
  if (samp_rate != GRAD_UPDATE_TIME){
    return KS_THROW("requested sample rate (%d) is different from the default (%d). not supported yet (or ever?)",
                    samp_rate, GRAD_UPDATE_TIME);
  }

  /* collect the list of modules to model */
  ks_eval_seqcollection_resetninst(grad_heat_handle.seqcollection_p);
  s = grad_heat_handle.play(max_encode_mode);
  KS_RAISE(s);

  /* reset current_playout field */
  for (i=0; i<grad_heat_handle.seqcollection_p->numseq; ++i) {

    if (!grad_heat_handle.seqcollection_p->seqctrlptr[i]) {
      return KS_THROW("NULL pointer to the %d-th sequence entry", i);
    }

    grad_heat_handle.seqcollection_p->seqctrlptr[i]->current_playout = 0;
  }

  struct _grad_list grads;
  s = _grad_list_init(&grads);
  if (s != SUCCESS) { goto fail; }

  /* iterate over the sequences of modules */
  for (i=0; i<grad_heat_handle.seqcollection_p->numplayouts; ++i) {

    if (!grad_heat_handle.seqcollection_p->seqplayouts[i]) {
      s = KS_THROW("NULL pointer to the %d-th sequence entry in playout order", i);
      goto fail;
    }

    /* log the sequence of module */
    /*ks_dbg("playout %d: %s, duration %d", i,
           grad_heat_handle.seqcollection_p->seqplayouts[i]->description,
           grad_heat_handle.seqcollection_p->seqplayouts[i]->duration);*/

    s = _grad_list_add_sequence(&grads, grad_heat_handle.seqcollection_p->seqplayouts[i]);
    if (s != SUCCESS) { goto fail; }
  }

  s = _grad_list_remove_duplicates(&grads);
  if (s != SUCCESS) { goto fail; }

  /*
  char boards[] = "XYZ";
  for (i=0; i<3; ++i) {
    ks_dbg("board %c has %d unique points", boards[i], grads.num_grads[i]);
  }
  ks_dbg("tot_time = %d", grads.max_time);*/

  /* allocate output arrays */
  {
    const int max_nout = grads.num_grads[X] + grads.num_grads[Y] + grads.num_grads[Z];
    FLOAT *p;
    FLOAT **ps[] = {p_time, ampl, ampl+1, ampl+2, pul_type, pul_type+1, pul_type+2};
    for (i=0; i<7; ++i) {
      /* free'ed by GE's code */
      p = (FLOAT*)malloc(max_nout * sizeof(FLOAT));
      if (!p) {
        s = KS_THROW("Allocation failed");
        goto fail;
      }
      *ps[i] = p;
    }
  }

#if EPIC_RELEASE > 26
  *num_iters = 1; /* TODO: we could use also numplayouts from seq collection */
#endif

  /* join by interpolation the corner points */
  s = _fillcp(grads.data, grads.num_grads, *p_time, ampl, pul_type, num_totpoints);
  if (s != SUCCESS) {
    KS_THROW("fillcp failed");
    goto fail;
  }

  /* common outro */
 fail:
  for (i=0; i<3; ++i) {
    free(grads.data[i]);
  }
  KS_RAISE(s);

  return SUCCESS;
}




static INT 
_fillcp( t_list *gradlist[3],
        INT *n,
        FLOAT *tout,
        FLOAT **aout,
        FLOAT **ptypout,
        INT *nout )
{
    INT i, status, maxk;
    INT nmax, k, done[3], ind[3], maxn[3], cont;
    FLOAT tcur, tmax;
    FLOAT slope, inter;
    FLOAT *ttmp[3], *atmp[3], *typtmp[3];

    status = 1;

    /* find maximum array size */
    nmax = 0;
    for ( i = 0 ; i < 3 ; i++ ) {
        if (n[i] > nmax) {
            nmax = n[i];
        }
    }

    nmax += 2;   /* make sure array is long enough to add a
                    trailing and leading zero */

    /* find max time */
    tmax = 0;
    for ( i = 0 ; i < 3 ; i++ ) {
        if (gradlist[i][n[i]-1].time > tmax) {
            tmax = gradlist[i][n[i]-1].time;
        }
    }

    /* allocate mem */
    for( i = 0 ; i < 3 ; i++ ) {
        ttmp[i] = (FLOAT *)AllocNode( nmax * sizeof(FLOAT) );
        if (ttmp[i] == NULL){
            fprintf( stderr, "Failure allocating ttmp[%d] array in fillcp\n",
                     i );
            epic_error(0,"Failure allocating ttmp[%d] array in fillcp",0,
                       EE_ARGS(1),INT_ARG,i);
            return FAILURE;
        }
        atmp[i] = (FLOAT *)AllocNode( nmax * sizeof(FLOAT) );
        if (atmp[i] == NULL){
            fprintf( stderr, "Failure allocating atmp[%d] array in fillcp\n",
                     i );
            epic_error(0,"Failure allocating atmp[%d] array in fillcp",0,
                       EE_ARGS(1),INT_ARG,i);
            return FAILURE;
        }

        typtmp[i] = (FLOAT *)AllocNode(nmax*sizeof(FLOAT));
        if (typtmp[i] == NULL){
            fprintf( stderr, "Failure allocating typtmp[%d] array in fillcp\n",
                     i );
            epic_error(0,"Failure allocating typtmp[%d] array in fillcp",0,
                       EE_ARGS(1),INT_ARG,i);
            return FAILURE;
        }

        if ( _fillArrays( ttmp[i], atmp[i], typtmp[i], gradlist[i], n[i],
                         tmax, maxn + i ) == FAILURE ) {
            fprintf( stderr, "Failure in fillArrays()\n" );
            epic_error(0,"%s failed",0,
                       EE_ARGS(1),STRING_ARG,"fillArrays");
            return FAILURE;
        }
    }

    /* set up for loop */
    k = 0;
    cont = 1;
    maxk = 0;
    for( i = 0 ; i < 3 ; i++ ) {
        ind[i] = 0;
        done[i] = 0;
        maxk += maxn[i];
    }

    /* while more points */
    while (cont == 1) {

        /* find min current time */
        tcur = 1e30;
        for ( i = 0 ; i < 3 ; i++ ) {
            if (ttmp[i][ind[i]] < tcur) {
                tcur = ttmp[i][ind[i]];
            }
        }

        /* for each axis */
        for(i=0;i<3;i++) {

            /* if time for the present axis is less than current */
            if (ttmp[i][ind[i]]<tcur) {

                /* an error has occured */
                status = -4;

            } /* end if */

            /* if time for the present axis is equal current */
            if (areSame(ttmp[i][ind[i]], tcur)) 
            {
                /* store in output array */
                aout[i][k] = atmp[i][ind[i]];
                ptypout[i][k] = typtmp[i][ind[i]];

                /* inc index for present axis */
                ind[i]++;

                /* if index exceeds axis limit */
                if (ind[i]>=maxn[i]) {

                    /* hold index and set done flag */
                    ind[i]--;
                    done[i]=1;

                } /* end if */
            } 
            else 
            {   /* else if time is greater */
                /* linear interpolate to fill output array */
                slope = (atmp[i][ind[i]]-atmp[i][ind[i]-1])/
                    (ttmp[i][ind[i]]-ttmp[i][ind[i]-1]);
                inter = atmp[i][ind[i]-1];

                aout[i][k] = slope * (tcur - ttmp[i][ind[i] - 1]) + inter;

                /* Store the pulse type of the previous point */
                ptypout[i][k] = typtmp[i][ind[i]-1];

            } /* end if (time for present axis) */

        } /* end for (each axis) */

        /* store time */
        tout[k] = tcur;

        /* inc output pointer */
        k++;

        /* check to see if all axes are done */
        if ((done[X] == 1) && (done[Y] == 1) && (done[Z] == 1)) {
            cont = 0;
        }

        /* if output index too large */
        if ( k > maxk ) {
            status = -5;
            cont = 0;
        }

    } /* end while */

    for( i = 0 ; i < 3 ; i++ ) {
        FreeNode(ttmp[i]);
        FreeNode(atmp[i]);
        FreeNode(typtmp[i]);
    }

    /* set nout */
    *nout = k;

    return(status);
}   /* end fillcp() */


/*
 *  fillArrays
 *  
 *  Type: Private Function
 *  
 *  Description:
 *  
 */
static STATUS 
_fillArrays( FLOAT *Ttmp,
            FLOAT *Atmp,
            FLOAT *Typtmp,
            t_list *List,
            INT num_elements,
            DOUBLE Tmax,
            INT *Maxn )
{
    INT I, K;

    /* fill temp arrays with Z data */
    K = 0;
    Ttmp[K] = 0;
    if (List[X].time<0) {
        printf ("Time element < 0\n");
        epic_error(0,"Time element %d < 0",0,EE_ARGS(1),INT_ARG,X);
        return FAILURE;
    }else if (List[X].time>0) {
        Atmp[X] = 0;                  /* waveform assumed to start at 0  */
        K = 1;

    }
    for(I=0;I<num_elements;I++) {
        Ttmp[K] = List[I].time;
        Atmp[K] = List[I].ampl;
        Typtmp[K] = List[I].ptype;
        K++;
    }
    if(List[num_elements-1].time<Tmax) {
        Ttmp[K] = Tmax;                 /* create point with time tmax */
        Atmp[K] = List[num_elements-1].ampl;            /* use last amp */
        Typtmp[K] = List[num_elements-1].ptype;         /* use last ptype */
        K++;
    }
    *Maxn = K;

    return SUCCESS;
}   /* end fillArrays() */


extern int gradHeatMethod;
extern int enforce_minseqseg;

STATUS ks_eval_gradlimits(int *newtime, 
                          KS_SEQ_COLLECTION *seqcollection,
                          const LOG_GRAD *log_grad,
                          STATUS (*play)(const INT max_encode_mode)) {
  STATUS s;

  if (!seqcollection) {
    return KS_THROW("NULL pointer to sequence collection");
  }

  /* Run the gradient heating model */
  ks_grad_heat_init(seqcollection, play);

  gradHeatMethod = TRUE;
  enforce_minseqseg = PSD_ON;

  if (seqcollection->numseq < 1 || seqcollection->numplayouts < 1) {
    return KS_THROW("No sequence entries to evaluate");
  }

  if (!seqcollection->seqctrlptr[0]) {
    return KS_THROW("NULL pointer to the first sequence entry");
  }

  /*ks_dbg("Running gradlimits on index %d", seqcollection->seqctrlptr[0]->handle.index);*/

  s = minseq(newtime,
             NULL, 0, NULL, 0, NULL, 0,
             log_grad, seqcollection->seqctrlptr[0]->handle.index,
             GRAD_UPDATE_TIME, 0 /* oldtime, unused */, 0, 0);

  /* revert to the old method for subsequent calls to minseq
     that are executed in prescan's entry points.
   */
  gradHeatMethod = FALSE;
  enforce_minseqseg = PSD_OFF;

  ks_grad_heat_reset();

  if (s != SUCCESS) {
    KS_THROW("minseq failed");
#ifndef SIM
    /* Don't return if in WTools */
    return FAILURE;
#endif

  }

  seqcollection->hwlimitsdone = 1;

  return SUCCESS;
}




extern _cvint _optr;
extern int minseqcable_t;
extern int minseqbusbar_t;

int ks_eval_rflimits(KS_SAR *sar, KS_SEQ_COLLECTION *seqcollection) {
  extern int tmin;
  int old_tmin;
  int i, j, k;
  int rfindx = 0;
  int numuniquerf = 0;
  int nettime = 0;
  int newtime_rfamp = 0;
  int newtime_gradheat = 0;
  int newtime_sar = 0;
  int newtime_allconstraints = 0;
  int dummy;
  double ave_sar = 0;
  double peak_sar = 0;
  double cave_sar = 0; /* Coil SAR */
  double b1rms = 0;

  STATUS status;
  if (seqcollection == NULL) {
    ks_error("%s: 2nd arg is NULL (KS_SEQ_COLLECTION)", __FUNCTION__);
    return KS_NOTSET;
  } else if (seqcollection->numseq < 1) {
    ks_error("%s: No sequence modules in 2nd arg (KS_SEQ_COLLECTION)", __FUNCTION__);
    return KS_NOTSET;
  }

  /********** copy to common struct arrays for grad and RF from the sequence collection **********/
  for (i = 0; i < seqcollection->numseq; i++) {
    for (j = 0; j < seqcollection->seqctrlptr[i]->gradrf.numrf; j++) {
      numuniquerf += seqcollection->seqctrlptr[i]->gradrf.rfptr[j]->rfpulse.num;
    }
  }

  nettime = ks_eval_seqcollection_gettotalduration(seqcollection);
  if (nettime <= 0) {
    return nettime;
  }

  RF_PULSE *myrfpulse = (RF_PULSE*)alloca(numuniquerf * sizeof(RF_PULSE));
  float *myrfflip = (float*)alloca(numuniquerf * sizeof(float));

  for (i = 0; i < seqcollection->numseq; i++) {

    for (j = 0; j < seqcollection->seqctrlptr[i]->gradrf.numrf; j++) {
        ks_eval_rf_relink(seqcollection->seqctrlptr[i]->gradrf.rfptr[j]);

        /* Point to the current KS_RF (e.g. excitation, refocusing etc) */
        KS_RF *myrf = seqcollection->seqctrlptr[i]->gradrf.rfptr[j];

        for (k = 0; k < myrf->rfpulse.num; k++) { /* or myrf->rfpulse.num */

          /* Deep copy of RF_PULSE structure (incl pointers to KS_RF fields .flip and .amp etc) */
          myrfpulse[rfindx] = myrf->rfpulse;

          myrfflip[rfindx] = myrf->flip * fabs(myrf->rfwave.locs[k].ampscale);

          /* Update the rfpulse.act_fa pointers so that are unique to each instance. */
          myrfpulse[rfindx].act_fa = &(myrfflip[rfindx]);      

          /* Each RF pulse instance is unique within a module so the total number is equal to the number 
            of times this sequence module is played out in the time period considered. */
          myrfpulse[rfindx].num = seqcollection->seqctrlptr[i]->nseqinstances;

          rfindx++;

        } /* for each RF instance */

      } /* for each unique KS_RF */

  }

  /* Protection against not understood values of minseqcable_t or minseqbusbar_t
     which can be -2147483648 (neg INT max). This is nonsense and out of valid CV range (by 1 value)
     and leads to download failure. Need to find where minseqcable_t and minseqbusbar_t are set.
   */
  if (minseqcable_t < 0 || minseqcable_t > 100 * nettime) {
    minseqcable_t = 0;
  }
  if (minseqbusbar_t < 0 || minseqbusbar_t > 100 * nettime) {
    minseqbusbar_t = 0;
  }


  /********** RF amplifier **********/
  /* tmin is used in minseqrfamp, this will fail if tmin is set to zero*/
  old_tmin = tmin;
  tmin = nettime;
  status = minseqrfamp(&newtime_rfamp, numuniquerf, myrfpulse, L_SCAN);
  tmin = old_tmin;
  /* ks_dbg("%s: minseqrfamp = %dus; nettime = %dus", __FUNCTION__, newtime_rfamp, nettime); */
  if (status != SUCCESS) {
    ks_error("%s: minseqrfamp() failed - Please reduce FA", __FUNCTION__);
    return KS_NOTSET;
  }

  /********** RF SAR **********/
#if EPIC_RELEASE >= 24
  status = maxsar(&newtime_sar,
                  &dummy, &ave_sar, &cave_sar, &peak_sar, &b1rms,
                  numuniquerf, myrfpulse, L_SCAN, nettime);
#else
  status = maxsar(&newtime_sar,
                  &dummy, &ave_sar, &cave_sar, &peak_sar,
                  numuniquerf, myrfpulse, L_SCAN, nettime);
#endif

  if (status != SUCCESS) {
    ks_error("%s: maxsar() failed", __FUNCTION__);
    return KS_NOTSET;
  }

  if (sar != NULL) {
    sar->average = ave_sar;
    sar->coil = cave_sar;
    sar->peak = peak_sar;
    sar->b1rms = b1rms;
  }

  newtime_allconstraints = IMax(3, nettime, newtime_rfamp, newtime_sar);

  if (sar == NULL) {
  /* Change description of optr only if `sar` (1st arg) == NULL (c.f. GEReq_eval_TR()->ks_eval_mintr() )
     This is because when we use ks_eval_mintr() we have .duration = .min_duration and hence nettime is equal
     to the bare sum of sequence module durations */
    char tmpstr[100];

    if (newtime_allconstraints == nettime) {
      sprintf(tmpstr, "TR [Not SAR/heat limited (%d)]", nettime);
    } else if (newtime_allconstraints == newtime_gradheat) {
      sprintf(tmpstr, "TR [Grad. heat limited (%d/%d)]", nettime, newtime_allconstraints);
    } else if (newtime_allconstraints == newtime_rfamp) {
      sprintf(tmpstr, "TR [RF amp limited (%d/%d)]", nettime, newtime_allconstraints);
    } else if (newtime_allconstraints == newtime_sar) {
      sprintf(tmpstr, "TR [SAR limited (%d/%d)]", nettime, newtime_allconstraints);
    }
    cvdesc(optr, tmpstr);  /* _optr declared as extern above */
  }

  /* return the maximum value (rounded up to nearest divisible by 8) */
  return RUP_GRD(newtime_allconstraints);

} /* ks_eval_rflimits */



STATUS ks_eval_hwlimits(int *newtime, 
                        KS_SEQ_COLLECTION *seqcollection,
                        const LOG_GRAD *log_grad,
                        STATUS (*play)(const INT max_encode_mode)) {

  STATUS s;

  s = ks_eval_gradlimits(newtime, seqcollection, log_grad, play);
  KS_RAISE(s);

  int newtime_rf;

  ks_eval_seqcollection_resetninst(seqcollection);
  play(AVERAGE_POWER);
  newtime_rf = ks_eval_rflimits(NULL, seqcollection);
  KS_RAISE(newtime_rf < 0 ? FAILURE : SUCCESS);
  *newtime = IMax(2, *newtime, newtime_rf);

  ks_eval_seqcollection_resetninst(seqcollection);
  play(MAXIMUM_POWER);
  newtime_rf = ks_eval_rflimits(NULL, seqcollection);
  KS_RAISE(newtime_rf < 0 ? FAILURE : SUCCESS);
  *newtime = IMax(2, *newtime, newtime_rf);

  return SUCCESS;
}




s64 ks_eval_getduration(KS_SEQ_COLLECTION *seqcollection, STATUS (*play)(const INT max_encode_mode)) {
  ks_eval_seqcollection_resetninst(seqcollection);
  play(AVERAGE_POWER);
  return ks_eval_seqcollection_gettotalduration(seqcollection);
}




int ks_eval_mintr(int nslices, KS_SEQ_COLLECTION *seqcollection, int (*play_loop)(int /* nslices */, int /* nargs */, void ** /*args */), int nargs, void **args) {

  /* must be run before each call to function pointer `play_loop()` to set all `seqctrl.nseqinstances` to 0 */
  ks_eval_seqcollection_resetninst(seqcollection);

  play_loop(nslices, nargs, args); /* => seqctrl.nseqinstances = # times each seq. module has been played out */

  return ks_eval_seqcollection_gettotalduration(seqcollection);

} /* ks_eval_mintr() */




int ks_eval_maxslicespertr(int TR, KS_SEQ_COLLECTION *seqcollection,
                           int (*play_loop)(int /*nslices*/, int /*nargs*/, void ** /*args*/), int nargs, void **args) {
  (void)seqcollection;

  int max_slquant1 = 0;
  int i, acqtime;
  for (i = 1; i < 1024; i++) {

    acqtime = play_loop(i, nargs, args);

    if (acqtime == KS_NOTSET) {
      return KS_NOTSET;
    }
    if (acqtime > TR) {
      return max_slquant1;
    }
    max_slquant1 = i;
  }

  return max_slquant1;
}




void ks_eval_seqcollection_resetninst(KS_SEQ_COLLECTION *seqcollection) {
  int i, j;
   if (seqcollection != NULL) {
    for (i = 0; i < seqcollection->numseq; i++) {
#ifndef IPG
      KS_SEQ_CONTROL *ctrl = seqcollection->seqctrlptr[i];

      if (!ctrl) { continue; }

      ctrl->nseqinstances = 0;
      for (j = 0; j < ctrl->gradrf.numrf; ++j) {
        ks_init_rtscalelog(&ctrl->gradrf.rfptr[j]->rfwave.rtscaling);
        ks_init_rtscalelog(&ctrl->gradrf.rfptr[j]->omegawave.rtscaling);
        ks_init_rtscalelog(&ctrl->gradrf.rfptr[j]->thetawave.rtscaling);
      }
      for (j = 0; j < ctrl->gradrf.numtrap; ++j) {
        ks_init_rtscalelog(&ctrl->gradrf.trapptr[j]->rtscaling);
      }
      for (j = 0; j < ctrl->gradrf.numwave; ++j) {
        ks_init_rtscalelog(&ctrl->gradrf.waveptr[j]->rtscaling);
      }
      for (j = 0; j < ctrl->gradrf.numwait; ++j) {
        ks_init_rtscalelog(&ctrl->gradrf.waitptr[j]->rtscaling);
      }
#endif
    }
    for (i = 0; i < seqcollection->numplayouts; i++) {
      seqcollection->seqplayouts[i] = NULL;
    }
    seqcollection->numplayouts = 0;
  }
}




s64 ks_eval_seqcollection_gettotalduration(KS_SEQ_COLLECTION *seqcollection) {
 int i;
 s64 nettime = 0;

  /* duration based on the sequence collection struct */
  for (i = 0; i < seqcollection->numseq; i++) {
    /*
    if (seqcollection->seqctrlptr[i]->duration > 0 && seqcollection->seqctrlptr[i]->nseqinstances == 0) {
      ks_error("%s: Sequence module #%d was not played out in sliceloop using ks_scan_playsequence()", __FUNCTION__, i);
      return KS_NOTSET;
    }
    */
    nettime += (s64)seqcollection->seqctrlptr[i]->duration * (s64)seqcollection->seqctrlptr[i]->nseqinstances;
  }

  if (nettime == 0) {
    ks_error("%s: Sum of durations of sequence modules used is 0", __FUNCTION__);
  }

  return nettime;
} /* ks_eval_seqcollection_gettotalduration() */


s64 ks_eval_seqcollection_gettotalminduration(KS_SEQ_COLLECTION *seqcollection) {
 int i;
 s64 nettime = 0;

  /* duration based on the sequence collection struct */
  for (i = 0; i < seqcollection->numseq; i++) {
    /*
    if (seqcollection->seqctrlptr[i]->min_duration > 0 && seqcollection->seqctrlptr[i]->nseqinstances == 0) {
      ks_error("%s: Sequence module #%d was not played out in sliceloop using ks_scan_playsequence()", __FUNCTION__, i);
      return KS_NOTSET;
    }
    */
    nettime += (s64)seqcollection->seqctrlptr[i]->min_duration * (s64)seqcollection->seqctrlptr[i]->nseqinstances;
  }

  if (nettime == 0) {
    ks_error("%s: Sum of min_durations of sequence modules used is 0", __FUNCTION__);
  }

  return nettime;
} /* ks_eval_seqcollection_gettotalminduration() */


STATUS ks_eval_seqcollection2rfpulse(RF_PULSE *rfpulse, KS_SEQ_COLLECTION *seqcollection) {
  int i, j, k;

  /* clear the activity bits for all (KS_MAXUNIQUE_RF) available RF pulse slots */
  for (i = 0; i < KS_MAXUNIQUE_RF; i++) {
    rfpulse[i].activity = 0;
  }

  i = 0;
  for (k = 0; k < seqcollection->numseq; k++) {

    for (j = 0; j < seqcollection->seqctrlptr[k]->gradrf.numrf; j++) {

      if (i >= KS_MAXUNIQUE_RF) {
        return ks_error("%s: too many RF pulses, recompilation is needed with an increased KS_MAXUNIQUE_RF",
                        __FUNCTION__);
      }

      ks_eval_rf_relink(seqcollection->seqctrlptr[k]->gradrf.rfptr[j]);
      rfpulse[i++] = seqcollection->seqctrlptr[k]->gradrf.rfptr[j]->rfpulse;
    }
  }

  return SUCCESS;

} /* ks_eval_seqcollection2rfpulse */



/*******************************************************************************************************
 *  Misc
 *******************************************************************************************************/

int ks_isIceHardware() {
#if EPIC_RELEASE >= 27
  if (isIceHardware() == TRUE) {
    return TRUE;
  }
#endif
return FALSE;
}




int ks_default_ssitime() {

  if (ks_isIceHardware() == TRUE) {
    return KS_DEFAULT_SSI_TIME_ICE;
  }

  return KS_DEFAULT_SSI_TIME;
/*
#define KS_DEFAULT_SSI_TIME 1500
#define KS_DEFAULT_SSI_TIME_MR750w 500
#define KS_DEFAULT_SSI_TIME_MR750 1000
#define KS_DEFAULT_SSI_TIME_MR450W 500
#define KS_DEFAULT_SSI_TIME_MR450 1000
#define KS_DEFAULT_SSI_TIME_PREMIER 500
#define KS_DEFAULT_SSI_TIME_OTHERWISE 1500
*/

}




STATUS ks_bitmask_set(unsigned int out,
                      const unsigned int in,
                      const unsigned int size,
                      const unsigned int offset) {

  if (size + offset >= sizeof(unsigned int)) {
    return ks_error("%s: the sum of size (%d) and offset (%d) must be less than %d", __FUNCTION__, size, offset, sizeof(unsigned int));
  }

  /* we need to copy an amount of bits equal to size from the input  */
  const unsigned int mask_in = (1ULL << size) - 1ULL;

  /* shift the input mask */
  const unsigned int mask_out = mask_in << offset;

  out = ((in & mask_in) << offset) | (out & ~mask_out);

  return SUCCESS;

} /* ks_bitmask_set() */


/**
 ****************************************************************************************************
 @brief #### Extract a specified amount of bits from the input with an offset
 * 
 */
unsigned int ks_bitmask_get(const unsigned int in,
                            const unsigned int size,
                            const unsigned int offset) {
  if (size + offset >= sizeof(unsigned int)) {
    ks_error("%s: the sum of size (%d) and offset (%d) must be less than %d", __FUNCTION__, size, offset, sizeof(unsigned int));
    return 0;
  }

  const unsigned int mask_in = ((1ULL << size) - 1ULL) << offset;

  return (in & mask_in) >> offset;
}




_cvfloat* opuser_from_number(_cvfloat* cv) {
  return cv;
}




unsigned int ks_calc_nextpow2(unsigned int n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}




int ks_calc_roundupms(int val) {
  return (((val + 999) / 1000) * 1000);
}




STATUS ks_calc_filter(FILTER_INFO *echortf, int tsp, int duration) {

  echortf->decimation = tsp / 2;
  echortf->bw = 1.0e3 / (tsp * 2.0);
  echortf->tsp = tsp; /* dwell time [us] */
  echortf->tdaq = duration;
  echortf->outputs = duration / echortf->tsp;
  echortf->fslot = KS_NOTSET; /* set this to an unusable value so we are forced to call setfilter() or GEReq_predownload_setfilter() */
  echortf->prefills = 0;
  echortf->taps = -1;

  if ((floor(echortf->tsp) < echortf->tsp) || (duration % (int) echortf->tsp)) {
    return ks_error("ks_calc_filter: tsp must be an integer number and 2nd arg (duration) must be even (divisible by 'tsp'), in units of us");
  }
  if (echortf->outputs % 2) {
    return ks_error("ks_calc_filter: the number of sample outputs must be even");
  }
  return SUCCESS;
}




STATUS ks_write_vector_bf(float* vec, uint32_t numel, const char* fname) {
  char embedfile_uid[512];
  #ifdef PSD_HW /* on MR-scanner */
    char outputdir_uid[512];
    char cmd[512];
    sprintf(outputdir_uid, "/usr/g/mrraw/kstmp/%010d/embed/", rhkacq_uid);
    sprintf(embedfile_uid, "%s/%s", outputdir_uid, fname);
    sprintf(cmd, "mkdir -p %s > /dev/null", outputdir_uid);
    system(cmd);
  #else
    sprintf(embedfile_uid, "./%s", fname);
  #endif
  FILE* fp = fopen(embedfile_uid, "wb");
  const uint32_t rank = 1;
  fwrite(&rank, sizeof(rank), 1, fp);
  fwrite(&numel, sizeof(numel), 1, fp);
  fwrite(vec, sizeof(float), numel, fp);
  fflush(fp);
  fclose(fp);
  return SUCCESS;
}




int ks_calc_bw2tsp(float bw) {
  int tsp;
  int sysmintsp = 2;

  tsp = ((int) floor((float) HALF_KHZ_USEC / bw / (float) sysmintsp + 0.5)) * sysmintsp;

  if (tsp < sysmintsp)
    tsp = sysmintsp;

  return tsp;
}




float ks_calc_tsp2bw(int tsp) {

  return ( (float) HALF_KHZ_USEC / (float) tsp );

}




float ks_calc_nearestbw(float bw) {
  int tsp;
  if (bw > 0) {
    tsp = (int) ks_calc_bw2tsp(bw); /* convert rBW to dwell time [us] */
    return (ks_calc_tsp2bw(tsp)); /* round rBW to exacly match the dwell time */
  } else {
    return bw;
  }
}




float ks_calc_lower_rbw(float rbw) {
  if (isNotSet(rbw)) {
    return rbw;
  }
  int tsp = ks_calc_bw2tsp(rbw);
  return ks_calc_nearestbw(500.0f / (tsp + 2));
}




float ks_calc_higher_rbw(float rbw) {
  int tsp = ks_calc_bw2tsp(rbw);
  if (tsp == 2 || isNotSet(rbw)) {
    return rbw;
  } else {
    return ks_calc_nearestbw(500.0f / (tsp - 2));
  }
}




float ks_calc_max_rbw(float ampmax, float fov) {
  float max_rbw = -1.0f;
  int dwelltime;
  for (dwelltime = 2; max_rbw < 0.0; dwelltime += 2) {
    float required_amp = 1.0/(GAM * fov * 0.1 * dwelltime * 0.000001);
    if (required_amp <= ampmax) {
      max_rbw = 500.0f / ((float) dwelltime);
    }
  }
  return max_rbw;
}




int ks_calc_trap_time2area(KS_TRAP* trap, float area) {
  float ramp_area = trap->amp * trap->ramptime / 2.0;
  float plateau_area = trap->amp * trap->plateautime;
  float slewrate = trap->amp / trap->ramptime;
  int time2area;
  if (area < ramp_area) {
    /* Target area reached on ramp */
    time2area = (int) sqrtf(area * 2.0 / slewrate);
  } else if (area < (ramp_area + plateau_area)) {
    /* Target area reached on plateau*/
    float area_remaining = area - ramp_area;
    time2area = trap->ramptime + (area_remaining / trap->amp);
  } else {
    /* Target area reached on ramp down */
    float area_remaining = area - ramp_area - plateau_area;
    time2area = (trap->amp - sqrtf(trap->amp*trap->amp  - 2*area_remaining*slewrate) ) / slewrate;
  }
  return time2area;
}




STATUS ks_calc_sliceplan(KS_SLICE_PLAN *slice_plan, int nslices, int slperpass) {

  return ks_calc_sliceplan_interleaved(slice_plan, nslices, slperpass, 2);

}




STATUS ks_calc_sliceplan_interleaved(KS_SLICE_PLAN *slice_plan, int nslices, int slperpass, int ninterleaves) {

  if (!slice_plan) {
    return ks_error("%s: invalid input, slice_plan is NULL", __FUNCTION__);
  }

  if (nslices <= 0 || slperpass <= 0 || ninterleaves <= 0) {
   return ks_error("%s: invalid input (nslices: %d, slperpass: %d, ninterleaves: %d)",
             __FUNCTION__, nslices, slperpass, ninterleaves);
  }

  slice_plan->nslices = nslices;
  slice_plan->npasses = CEIL_DIV(nslices, slperpass);
  slice_plan->nslices_per_pass = slperpass;

  int nslicesthispass[slice_plan->npasses];
  int sllocthispass[slice_plan->npasses][slperpass];
  int i, p, s, k, t, interleaf;
  /*
  - slice_plan->npasses: a.k.a. number of acquisitions (standard GE CV is: `acqs`)
  - slperpass: maximum number of slices in a pass (some passes may have fewer slices)
  - nslicesthispass[slice_plan->npasses]: Array where each element indicates how many slices to acquired in this pass
  - sllocthispass[slice_plan->npasses][slperpass]: Spatial location index into the prescribed slice stack, i.e. all slices
    for current pass and pass_sliceindx (spatially sorted)
  */

  if (slice_plan->npasses == slice_plan->nslices) {
    /* Sequential scanning, opirmode = 1 (i.e. fill k-space fully for each slice, then change slice). We don't want
       to acquire them sequentially in space though to avoid cross talk. Therefore, interleaves are made over the slice stack. */

    p = 0;
    for (interleaf = 0; interleaf < ninterleaves; interleaf++) {
      /* interleaf: slice interleaves over stack (typically odd/even) */
      for (i = 0; i < CEIL_DIV(slice_plan->nslices, ninterleaves); i++) {
        k = interleaf + i * ninterleaves;
        slice_plan->acq_order[p].slloc  = k; /* Spatial location index into the prescribed slice stack */
        slice_plan->acq_order[p].slpass = p; /* pass index */
        slice_plan->acq_order[p].sltime = 0; /* time index in current pass = 0 */
        p++;
      }
    }

  } else {
    /* Standard, interleaved scanning, opirmode = 0. Interleaves are made within passes. */

    for (p = 0; p < slice_plan->npasses; p++) {
      nslicesthispass[p] = 0;
      for (s = 0; s < slperpass; s++) {
        sllocthispass[p][s] = -1;
        if (s * slice_plan->npasses + p < nslices) {
          nslicesthispass[p]++;
          sllocthispass[p][s] = p + (s * slice_plan->npasses);
        }
      }
    }

    for (p = 0; p < slice_plan->npasses; p++) {
      t = 0; /* linearly increasing time index in current pass */
      for (interleaf = 0; interleaf < ninterleaves; interleaf++) {
        /* interleaf: slice interleaves *within* each pass (typically odd/even) */
        for (i = 0; i < CEIL_DIV(nslicesthispass[p], ninterleaves); i++) {
          k = interleaf + i * ninterleaves; /* pass_sliceindx */
          if (k < nslicesthispass[p]) {
            slice_plan->acq_order[sllocthispass[p][k]].slloc  = sllocthispass[p][k]; /* Spatial location index into the prescribed slice stack */
            slice_plan->acq_order[sllocthispass[p][k]].slpass = p; /* pass index */
            slice_plan->acq_order[sllocthispass[p][k]].sltime = t++; /* time index in current pass. 0->nslicesthispass[p] */
          }
        }
      }
    }

  } /* sequential scanning or not */


  return SUCCESS;
}

STATUS ks_calc_sliceplan_centerout(KS_SLICE_PLAN *slice_plan, int nslices) {

  if (!slice_plan) {
    return ks_error("%s: invalid input, slice_plan is NULL", __FUNCTION__);
  }
  if (nslices <= 0) {
    return ks_error("%s: invalid input (nslices: %d)", __FUNCTION__, nslices);
  }

  /* Center-out interleaved slice order to minimize SMS slice cross-talk.
   *
   * Builds two interleaves (0-indexed):
   *   il1: even indices [0, 2, 4, ...]
   *   il2: odd  indices [1, 3, 5, ...]
   * Each is sorted by ascending distance from the spatial center using a stable
   * insertion sort (ties preserve the original spatial order).
   * Final acquisition order: il1 followed by il2.
   *
   * Matches the MATLAB scheme (1-indexed):
   *   center = N/2;
   *   interleave1 = sort(1:2:N by |val - center|);
   *   interleave2 = sort(2:2:N by |val - center|);
   *   my_slices = [interleave1, interleave2];
   */
  const int n1     = (nslices + 1) / 2; /* count of even-indexed slices: 0, 2, 4, ... */
  const int n2     = nslices / 2;       /* count of odd-indexed  slices: 1, 3, 5, ... */
  const int center = nslices / 2 - 1;  /* 0-indexed center (MATLAB N/2 in 1-indexed = N/2-1 in 0-indexed) */

  int il1[n1], il2[n2];
  int i, j, t, slloc;

  for (i = 0; i < n1; i++) il1[i] = 2 * i;
  for (i = 0; i < n2; i++) il2[i] = 2 * i + 1;

  /* Stable insertion sort by ascending |val - center|; ties preserve original order */
  for (i = 1; i < n1; i++) {
    int key = il1[i], key_d = abs(key - center);
    for (j = i - 1; j >= 0 && abs(il1[j] - center) > key_d; j--)
      il1[j + 1] = il1[j];
    il1[j + 1] = key;
  }
  for (i = 1; i < n2; i++) {
    int key = il2[i], key_d = abs(key - center);
    for (j = i - 1; j >= 0 && abs(il2[j] - center) > key_d; j--)
      il2[j + 1] = il2[j];
    il2[j + 1] = key;
  }

  slice_plan->nslices          = nslices;
  slice_plan->npasses          = 1;
  slice_plan->nslices_per_pass = nslices;

  /* Assign sltime (acquisition time index within the single pass) */
  t = 0;
  for (i = 0; i < n1; i++) {
    slloc = il1[i];
    slice_plan->acq_order[slloc].slloc  = slloc;
    slice_plan->acq_order[slloc].slpass = 0;
    slice_plan->acq_order[slloc].sltime = t++;
  }
  for (i = 0; i < n2; i++) {
    slloc = il2[i];
    slice_plan->acq_order[slloc].slloc  = slloc;
    slice_plan->acq_order[slloc].slpass = 0;
    slice_plan->acq_order[slloc].sltime = t++;
  }

  return SUCCESS;

}

STATUS ks_calc_sliceplan_sequential3D(KS_SLICE_PLAN *slice_plan, int nslices, int slperpass, int ninterleaves) {

  if (!slice_plan) {
    return KS_THROW("Invalid input, slice_plan is NULL");
  }

  if (nslices <= 0 || slperpass <= 0 || ninterleaves <= 0) {
   return KS_THROW("Invalid input (nslices: %d, slperpass: %d, ninterleaves: %d)", nslices, slperpass, ninterleaves);
  }

  slice_plan->nslices = nslices;
  slice_plan->npasses = CEIL_DIV(nslices, slperpass);
  slice_plan->nslices_per_pass = slperpass;
  
  
  int i, p, t, s, interleaf;

  /*
  - slice_plan->npasses: a.k.a. number of acquisitions (standard GE CV is: `acqs`)
  - slperpass: number of kz locations in a slab (pass=slab for sequential)
  */
 

  /* Sequential scanning, opirmode = 1 (i.e. fill k-space fully for each slab, then change slab). We might not want 
  to acquire them sequentially in space though to avoid cross talk. Therefore, interleaves are made over the slabs. */
  s = 0;
  for (interleaf = 0; interleaf < ninterleaves; interleaf++) {
    for (p = interleaf; p < slice_plan->npasses; p += ninterleaves) { /* Slabs */
      for (i = p * slperpass; i < (p+1) * slperpass; i++){ /* slices in slab */
        t = s % slperpass;
        slice_plan->acq_order[i].slloc  = s; /* Spatial location index into the prescribed slice stack */
        slice_plan->acq_order[i].slpass = p; /* pass index */
        slice_plan->acq_order[i].sltime = t; /* time index in current pass */
        s++;
      }
    }
  }

  return SUCCESS;
} /* ks_calc_sliceplan_sequential3D() */


STATUS ks_calc_sliceplan_sms(KS_SLICE_PLAN *slice_plan, int nslices, int slperpass, int multiband_factor) {
  int slices_per_band, passes_per_band;
  int i;
  STATUS status;

  /* sanitize the input */
  multiband_factor = multiband_factor < 1 ? 1 : multiband_factor;
  nslices = nslices < 0 ? 1 : nslices;
  slperpass = slperpass < 0 ? 1 : slperpass;

  if (slperpass > nslices)
    slperpass = nslices;

  /* no SMS?, fall back to standard slice plan */
  if (multiband_factor == 1) {
    status = ks_calc_sliceplan(slice_plan, nslices, slperpass);
    return status;
  }

  /* a 'band' is a group of slices out of the prescribed slices (opslquant), where the #slices in the stack is equal to
     opslquant / multiband_factor. The stack does only cover 1/multiband_factor of the entire slice FOV (i.e. not interleaved).
     The exact number of slices in each stack is ceil(opslquant/multiband_factor), which may often lead to that more slices than
     prescribed need to be acquired for divisibility */
  slices_per_band = CEIL_DIV(nslices, multiband_factor);

  /* number of passes per TR for one slice stack */
  passes_per_band = CEIL_DIV(slices_per_band, slperpass);

  if (passes_per_band == 1) {
    /* we can fit all slices in the stack in 1 pass (acquisition), i.e. one TR. Do SMS-specific slice interleaving */
    ks_calc_slice_acquisition_order_smssingleacq(slice_plan->acq_order, slices_per_band);
  } else {
    /* need 2+ acqs to acquire the 'slices_per_band' slices. Do standard slice interleaving */
    ks_calc_sliceplan(slice_plan, slices_per_band, slperpass);
  }

  /* copy to stacks 2->multiband_factor */
  slice_plan->nslices = slices_per_band * multiband_factor;
  slice_plan->npasses = passes_per_band * multiband_factor;

  for (i = slices_per_band; i < slice_plan->nslices; i++) {
    int slice_in_stack = i % slices_per_band;
    int stack = i / slices_per_band;
    slice_plan->acq_order[i].slloc  = i;
    slice_plan->acq_order[i].slpass = slice_plan->acq_order[slice_in_stack].slpass + stack * passes_per_band;
    slice_plan->acq_order[i].sltime = slice_plan->acq_order[slice_in_stack].sltime;
  }

  return SUCCESS;
}




STATUS ks_sms_check_divisibility(int *inpass_interleaves,
                                  const int sms_factor, const int nslices_per_pass, const int nslices) {

  if ((sms_factor > 1) && (!(nslices_per_pass % 2))) {
    int npasses = CEIL_DIV(nslices, nslices_per_pass);
    int adjusted_ileaves = 0;
    if (areSame((float)nslices/nslices_per_pass,(float)npasses)) {
      /* Check if adjusting the number of interleaves can help */
      int i;
      for (i = 4; i > 1; i--) {
        int y = CEIL_DIV(nslices_per_pass, i);
        float x = (float)(nslices_per_pass) / (float)(y*(i-1) + (y-1));
        if (areSame(x,1.0)) {
          *inpass_interleaves = i;
          adjusted_ileaves = 1;
        }    
      }
    }
    if (!adjusted_ileaves) {
      int down = (nslices_per_pass - 1) * npasses * sms_factor; 
      int up = (nslices_per_pass + 1) * npasses * sms_factor; 
      return ks_error("#slices not compatible with SMS, try %d or %d.", down, up);
    } else {
      ks_dbg("%s: Adjusted inpass_interleaves to %d, for SMS divisibillity.", __FUNCTION__, *inpass_interleaves);
    }
  }

  return SUCCESS;
}




int ks_calc_sms_min_gap(DATA_ACQ_ORDER *dacq, int nslices) {
  int min_gap = nslices;

  int i;
  for (i=1; i<nslices; i++) {
    int gap = abs(dacq[i-1].sltime - dacq[i].sltime);
    if (gap < min_gap) {
      min_gap = gap;
    }
  }

  int last_to_first = nslices - dacq[nslices-1].sltime;
  if (last_to_first < min_gap) {
    min_gap = last_to_first;
  }

  return min_gap;
}




int ks_calc_slice_acquisition_order_smssingleacq_impl(DATA_ACQ_ORDER *dacq, int nslices, int interleave) {

  const int ngroups = (nslices + interleave -1)/interleave;
  const int ngroups_full = nslices%ngroups ? nslices%ngroups : ngroups;

  int g, s, t=0;
  for (s=0; s<interleave; ++s) {
    for (g=0; g<ngroups_full; ++g) {
      int i = g*interleave + s;
      dacq[i].slloc  = i;
      dacq[i].slpass = 0;
      dacq[i].sltime = t++;
    }

    if (s+1 == interleave) { continue; }

    for (g=ngroups_full; g<ngroups; ++g) {
      int i = ngroups_full + g*(interleave-1) + s;
      dacq[i].slloc  = i;
      dacq[i].slpass = 0;
      dacq[i].sltime = t++;
    }
  }

  const int sl_in_last_group = nslices%interleave ? interleave-1 : interleave;
  const int sl_pairs_to_switch =  (sl_in_last_group-1)/2;
  int i;
  for(i=1; i<=sl_pairs_to_switch; ++i) {
    int tmp;
    tmp = dacq[nslices-i].sltime;
    dacq[nslices-i].sltime = dacq[nslices-sl_in_last_group+i].sltime;
    dacq[nslices-sl_in_last_group+i].sltime = tmp;
    }

  return 1;
}




int ks_calc_slice_acquisition_order_smssingleacq(DATA_ACQ_ORDER *dacq, int nslices) {

  return nslices % 2 ?
    ks_calc_slice_acquisition_order_smssingleacq_impl(dacq, nslices, 2) :
    ks_calc_slice_acquisition_order_smssingleacq_impl(dacq, nslices, 3);
}




float ks_calc_integrate_coords(float* t, float* G, int num_coords) {
  int i;
  if (num_coords < 2) {
    return ks_error("%s: At least two coords must be provided", __FUNCTION__);
  }

  for (i = 1; i < num_coords; i++) {
    if (t[i] < t[i-1]) {
      return ks_error("%s: Coords must be in time order", __FUNCTION__);
    }
  }
  float sum = 0.0;
  for (i = 0; i < (num_coords -1); i++) {
    sum += (t[i+1] - t[i])/2 * (G[i] + G[i+1]);
  }
  return sum;
}




int ks_get_center_encode(const KS_PHASEENCODING_PLAN * const peplan,
                             const int shot,
                             const int yres,
                             const int zres) {

  int best_encode = -1;
  KS_PHASEENCODING_COORD closest_coord;
  float closest_distance = 1.0f/0.0f; /* +Inf */

  const float y_center = (yres - 1) / 2.0;
  const float z_center = (zres - 1) / 2.0;

  int encode = 0;
  for (; encode < peplan->encodes_per_shot; encode++) {
    KS_PHASEENCODING_COORD coord = ks_phaseencoding_get(peplan, encode, shot);

    const float distance = (zres == KS_NOTSET || coord.kz == KS_NOTSET)  ?
      fabs(coord.ky - y_center) :
      (coord.ky - y_center)*(coord.ky - y_center) + (coord.kz - z_center)*(coord.kz - z_center);

    if (distance < closest_distance) {
        best_encode = encode;
        closest_coord = coord;
        closest_distance = distance;
    }
  }

  return best_encode;

} /* ks_get_center_encode() */





int ks_get_center_encode_linearsweep(const KS_KSPACE_ACQ* kacq,
                                     const int etl /* echoes per shot */,
                                     const KS_PF_EARLYLATE pf_earlylate_te) {

  const KS_PEPLAN_SHOT_DISTRIBUTION shot_dist = ks_peplan_distribute_shots(etl, kacq->num_coords, KS_NOTSET /* center not known */);
  const int searched_encode_low_to_high = ks_peplan_find_center_from_linear_sweep(kacq->coords, NULL, kacq->num_coords, MTF_Y, 1, shot_dist.shots, 0 /* start from zero */);
  const int searched_encode_high_to_low = ks_peplan_find_center_from_linear_sweep(kacq->coords, NULL, kacq->num_coords, MTF_Y, -1, shot_dist.shots, 0 /* start from zero */);
/*   ks_dbg("searched_encode_low_to_high = %d, searched_encode_high_to_low = %d", searched_encode_low_to_high, searched_encode_high_to_low); */
  int out = searched_encode_low_to_high;
  if (pf_earlylate_te == KS_PF_EARLY) {
    out = IMin(2, searched_encode_low_to_high, searched_encode_high_to_low);
  } else if (pf_earlylate_te == KS_PF_LATE) {
    out = IMax(2, searched_encode_low_to_high, searched_encode_high_to_low);
  }
  return out;

} /* ks_get_center_encode_linearsweep() */




STATUS ks_eval_flowcomp_slice(KS_SELRF* selrfexc, KS_TRAP* fcompslice) {
  STATUS status;

  float Gs = 0.0;
  int rs = 0;
  int Ts = 0;
  if (selrfexc->grad.duration > 0) {
    Gs = selrfexc->grad.amp;
    rs = selrfexc->grad.ramptime;
    Ts = selrfexc->rf.iso2end_subpulse;
  } else {
    Gs = selrfexc->gradwave.max_amp;
    rs = Gs/selrfexc->gradwave.abs_max_slew;
    Ts = selrfexc->rf.iso2end_subpulse - rs;
  }

  float m_s = Gs * ((Ts*Ts)/2.0 + (rs*rs)/6.0 + (Ts*rs)/2.0);
  float A_s = -selrfexc->postgrad.area; 
 
  // Binary search initialization
  float low = selrfexc->postgrad.area;
  float high = selrfexc->postgrad.area * 2.0; 
  float mid = 0.0;
  float netM1_mid = 100.0;
  float netM1_low = 0.0;
  float m1 = 0.0;
  float m2 = 0.0;
  int iter = 0;

  while ((fabs(high) - fabs(low)) >= 0.000001 && iter < 50) {

    /* LOW */
    selrfexc->postgrad.area = low;
    fcompslice->area = -(selrfexc->postgrad.area + A_s);

    status = ks_eval_trap(fcompslice, "fcompslice2");
    KS_RAISE(status);  
    status = ks_eval_trap(&selrfexc->postgrad, "rfexc.reph_fc1");
    KS_RAISE(status);

    m1 = selrfexc->postgrad.area * (Ts + rs + selrfexc->postgrad.duration/2);
    m2 = fcompslice->area * (Ts + rs + selrfexc->postgrad.duration + fcompslice->duration/2);
    netM1_low = m_s + m1 + m2;

    /* MID */
    mid = (low + high) / 2.0;
    selrfexc->postgrad.area = mid;
    fcompslice->area = -(selrfexc->postgrad.area + A_s);

    status = ks_eval_trap(fcompslice, "fcompslice2");
    KS_RAISE(status);  
    status = ks_eval_trap(&selrfexc->postgrad, "rfexc.reph_fc1");
    KS_RAISE(status);

    m1 = selrfexc->postgrad.area * (Ts + rs + selrfexc->postgrad.duration/2);
    m2 = fcompslice->area * (Ts + rs + selrfexc->postgrad.duration + fcompslice->duration/2);
    netM1_mid = m_s + m1 + m2;

    /* EVAL */
    if (areSame(netM1_mid,0.0)) {
      break;
    } else if (netM1_low * netM1_mid > 0) {
      low = mid;
    } else {
      high = mid;
    }
    iter++;

  }

  selrfexc->postgrad.area = (fabs(netM1_mid) > fabs(netM1_low)) ? low : mid;
  fcompslice->area = -(selrfexc->postgrad.area + A_s);
  status = ks_eval_trap(fcompslice, "fcompslice2");
  KS_RAISE(status);  
  status = ks_eval_trap(&selrfexc->postgrad, "rfexc.reph_fc1");
  KS_RAISE(status);

  if (selrfexc->grad.duration > 0) {
    selrfexc->rf2grad_end = selrfexc->grad.ramptime + selrfexc->postgrad.duration;
  } else {
    selrfexc->rf2grad_end = rs + selrfexc->postgrad.duration;
  }

  return SUCCESS;
} /* ks_eval_flowcomp_slice() */




STATUS ks_eval_flowcomp_phase(KS_TRAP *fcphase, const int min_dur, const float M1, const char *desc) { 
  STATUS status;
  int dur = min_dur;
  float required_area = 1.0;
  float actual_area = 0.0;
  int iter = 0;

  while (required_area > actual_area && iter < 500) {

    fcphase->duration = dur;
    status = ks_eval_trap_constrained_time_maxarea(fcphase, desc, ks_syslimits_ampmax(loggrd), ks_syslimits_slewrate(loggrd), 1e10, 0);
    KS_RAISE(status);

    required_area = M1 / fcphase->duration;
    actual_area = fcphase->area;

    dur += 16;
    iter++;
  }

  return SUCCESS;
} /* ks_eval_flowcomp_phase() */




void ks_print_seqcollection(KS_SEQ_COLLECTION *seqcollection, FILE *fp) {
  int i;

  /* duration based on the sequence collection struct */
  fprintf(fp, "\n----------------------------------------------\n");
  fprintf(fp, "Sequence modules in the sequence collection:\n");
  for (i = 0; i < seqcollection->numseq; i++) {
    fprintf(fp, "%s: %dx %d us = %u\n", seqcollection->seqctrlptr[i]->description, seqcollection->seqctrlptr[i]->nseqinstances, seqcollection->seqctrlptr[i]->duration,
      (unsigned int) (seqcollection->seqctrlptr[i]->nseqinstances * seqcollection->seqctrlptr[i]->duration));
  }
  fprintf(fp, "Sum of durations: %lld us\n", (s64)  (ks_eval_seqcollection_gettotalduration(seqcollection)));
  fprintf(fp, "----------------------------------------------\n\n");
  fflush(fp);

  return;
} /* ks_print_seqcollection() */



void ks_print_sliceplan(const KS_SLICE_PLAN slice_plan, FILE *fp) {
  int i;

  fprintf(fp, "\nSlice Plan:\n");
  fprintf(fp, "=============================================\n");
  fprintf(fp, "Number of slices: %d\n", slice_plan.nslices);
  fprintf(fp, "Number of passes: %d\n", slice_plan.npasses);
  fprintf(fp, "Slices per pass:  %d\n", slice_plan.nslices_per_pass);
  fprintf(fp, "\nslpass\tsltime\tslloc\n");
  fprintf(fp, "---------------------------------------------\n");
  for (i = 0; i < slice_plan.nslices; i++) { /* i = spatial location */
    fprintf(fp, "%03d   \t%03d   \t%03d\n", slice_plan.acq_order[i].slpass, slice_plan.acq_order[i].sltime, slice_plan.acq_order[i].slloc);
  }
  fprintf(fp, "=============================================\n\n");

}




void ks_print_scaninfo(const SCAN_INFO *scan_info, int nslices, const char *desc, FILE *fp) {
  int i;

  fprintf(fp, "=============================================\n\n");

  if (desc != NULL) {
    fprintf(fp, "\nScan info %s (%d slices):\n", desc, nslices);
  } else {
    fprintf(fp, "\nScan info (%d slices):\n", nslices);
  }
  fprintf(fp, "---------------------------------------------\n");
  fprintf(fp, "rloc\t\tphasoff\t\ttloc\t\trloc_shift\t\tphasoff_shift\t\ttloc_shift\n");
  for (i = 0; i < nslices; i++) {
    fprintf(fp, "%.3f\t\t%.3f\t\t%.3f\t%.3f\t%.3f\t%.3f\n",
                 scan_info[i].oprloc,
                 scan_info[i].opphasoff,
                 scan_info[i].optloc,
                 scan_info[i].oprloc_shift,
                 scan_info[i].opphasoff_shift,
                 scan_info[i].optloc_shift);
  }
  fprintf(fp, "Rotation matrix: [");
  for (i=0;i<9;i++) {
    fprintf(fp, "%.2f", scan_info[0].oprot[i]);
    if (i < 8) {
      fprintf(fp, ", ");
    }
  }
  fprintf(fp, "]\n");
  fprintf(fp, "=============================================\n\n");

}




void ks_print_waveform(const KS_WAVEFORM waveform, const char *filename, int res) {

  int i;
  FILE *asciiWaveFile;
  asciiWaveFile = fopen(filename, "w");

  if (asciiWaveFile == NULL) {
    return;
  }

  for (i = 0; i < res; i++) {
    fprintf(asciiWaveFile, "%d %f\n", i, waveform[i]); fflush(asciiWaveFile);
  }

  fclose(asciiWaveFile); /* close ascii file */
}




void ks_print_wave_part(const KS_WAVE* const wave, const char* filename, int startidx, int numel) {
  FILE *fp = fopen(filename, "ab");
  if (fp == NULL) {
    return;
  }
  if (startidx < 0 || startidx >= wave->res) {
    ks_error("%s: startidx (%d) is not in [0, %d]", __FUNCTION__, startidx, wave->res-1);
    return;
  }
  int stopidx = startidx + numel;
  if (stopidx < startidx || stopidx >= wave->res) {
    ks_error("%s: stopidx (%d) is not in [%d, %d]", __FUNCTION__, stopidx, startidx, wave->res-1);
    return;
  }
  fwrite(&wave->waveform[startidx], sizeof(float), numel, fp);
  fflush(fp);
  fclose(fp);
}




void ks_print_readwave(const KS_READWAVE* const readwave, const char* filename) {
  FILE *fp;
  int i = 0;
  fp = fopen(filename, "w");

  if (fp == NULL) {
    return;
  }

  for (i = (readwave->acqdelay / GRAD_UPDATE_TIME);
       i < (readwave->acqdelay + readwave->acq.duration) / GRAD_UPDATE_TIME;
       i++) {
    fprintf(fp, "%f, ", readwave->grad.waveform[i]);
  }
  fflush(fp);
  fclose(fp); /* close ascii file */
}




void ks_print_wave(const KS_WAVE* const wave, const char* filename) {
  FILE *fp;
  int i = 0;
  fp = fopen(filename, "w");

  if (fp == NULL) {
    return;
  }

  for (i = 0; i < wave->res; i++) {
    fprintf(fp, "%f, ", wave->waveform[i]);
  }
  fflush(fp);
  fclose(fp); /* close ascii file */
}




void ks_print_read(KS_READ a, FILE *fp) {
  if (a.description == NULL || a.duration == 0)
    return;
  fprintf(fp, "%s.duration: %d\n", a.description, a.duration);
  fprintf(fp, "%s.rbw: %g\n", a.description, a.rbw);
  fprintf(fp, "%s.filt.decimation: %g\n", a.description, a.filt.decimation);
  fprintf(fp, "%s.filt.tdaq: %d\n", a.description, a.filt.tdaq);
  fprintf(fp, "%s.filt.bw: %g\n", a.description , a.filt.bw);
  fprintf(fp, "%s.filt.tsp: %g\n", a.description, a.filt.tsp);
  fprintf(fp, "%s.filt.outputs: %d\n", a.description, a.filt.outputs);
  fprintf(fp, "%s.filt.prefills: %d\n", a.description, a.filt.prefills);
  fprintf(fp, "%s.filt.taps: %d\n", a.description, a.filt.taps);
  fflush(fp);
}




void ks_print_trap(KS_TRAP t, FILE *fp) {
  if (t.description == NULL || t.duration == 0)
    return;
  fprintf(fp, "%s.amp: %g\n", t.description, t.amp);
  fprintf(fp, "%s.ramptime: %d\n", t.description, t.ramptime);
  fprintf(fp, "%s.plateautime: %d\n", t.description, t.plateautime);
  fprintf(fp, "%s.duration: %d\n", t.description , t.duration);
  fprintf(fp, "%s.area: %g\n", t.description, t.area);
  fflush(fp);
}




void ks_print_readtrap(KS_READTRAP r, FILE *fp) {
    if (r.grad.description == NULL || r.grad.duration == 0)
    return;
  fprintf(fp, "\nKS_READTRAP (%s):\n", r.grad.description);
  fprintf(fp, "fov: %g\n", r.fov);
  fprintf(fp, "res: %d\n", r.res);
  fprintf(fp, "rampsampling: %d\n", r.rampsampling);
  fprintf(fp, "nover: %d\n", r.nover);
  fprintf(fp, "acqdelay: %d\n", r.acqdelay);
  fprintf(fp, "area2center: %g\n", r.area2center);
  fprintf(fp, "time2center: %d\n", r.time2center);
  fflush(fp);
  ks_print_read(r.acq, fp);
  ks_print_trap(r.grad, fp);
  if (r.omega.duration > 0) {
    fprintf(fp, "omega: {");
    int i = 0;
    for (i = 0; i < r.omega.res; i++) {
      fprintf(fp, "%f, ", r.omega.waveform[i]);
    }
    fprintf(fp, "}");
    fflush(fp);
  }
}




void ks_print_phaser(KS_PHASER p, FILE *fp) {
  if (p.grad.description == NULL || p.grad.duration == 0)
    return;
  fprintf(fp, "\nKS_PHASER (%s):\n", p.grad.description);
  fprintf(fp, "fov: %g\n", p.fov);
  fprintf(fp, "res: %d\n", p.res);
  fprintf(fp, "nover: %d\n", p.nover);
  fprintf(fp, "R: %d\n", p.R);
  fprintf(fp, "nacslines: %d\n", p.nacslines);
  fprintf(fp, "areaoffset: %g\n", p.areaoffset);
  fprintf(fp, "numlinestoacq: %d\n", p.numlinestoacq);
  fflush(fp);
  ks_print_trap(p.grad, fp);
}




void ks_print_gradrfctrl(KS_GRADRFCTRL gradrfctrl, FILE *fp) {
  int i;

  for (i = 0; i < gradrfctrl.numrf; i++) {
    fprintf(fp, "RF[%d] '%s': %d times\n", i, gradrfctrl.rfptr[i]->rfwave.description, gradrfctrl.rfptr[i]->rfwave.base.ninst);
  }
  for (i = 0; i < gradrfctrl.numtrap; i++) {
    fprintf(fp, "TRAP[%d] '%s': [%d, %d, %d] times on [X,Y,Z]\n", i, gradrfctrl.trapptr[i]->description, gradrfctrl.trapptr[i]->gradnum[XGRAD], gradrfctrl.trapptr[i]->gradnum[YGRAD], gradrfctrl.trapptr[i]->gradnum[ZGRAD]);
  }
  for (i = 0; i < gradrfctrl.numwave; i++) {
    fprintf(fp, "WAVE[%d] '%s': [%d, %d, %d] times on [X,Y,Z]\n", i, gradrfctrl.waveptr[i]->description, gradrfctrl.waveptr[i]->gradnum[XGRAD], gradrfctrl.waveptr[i]->gradnum[YGRAD], gradrfctrl.waveptr[i]->gradnum[ZGRAD]);
  }

}




/*-*/

void ks_print_epi(KS_EPI s, FILE *fp) {
  fprintf(fp, "\nKS_EPI (%s):\n", s.read.grad.description);
  fprintf(fp, "etl: %d\n", s.etl);
  fprintf(fp, "read_spacing: %.2f [ms]\n", s.read_spacing / 1000.0);
  fprintf(fp, "duration: %.2f [ms]\n", s.duration / 1000.0);
  fprintf(fp, "time2center: %.2f [ms]\n", s.time2center / 1000.0);
  ks_print_readtrap(s.read, fp);
  ks_print_trap(s.readphaser, fp);
  ks_print_trap(s.blip, fp);
  ks_print_phaser(s.blipphaser, fp);
} /* ks_print_epi */


/*-*/

void ks_print_rfpulse(RF_PULSE rfpulse, FILE *fp) {
  fprintf(fp, "     rfpulse.pw: %d\n", *(rfpulse.pw));
  fprintf(fp, "     rfpulse.amp: %g\n", *(rfpulse.amp));
  fprintf(fp, "     rfpulse.abswidth: %g\n", rfpulse.abswidth);
  fprintf(fp, "     rfpulse.effwidth: %g\n", rfpulse.effwidth);
  fprintf(fp, "     rfpulse.area: %g\n", rfpulse.area);
  fprintf(fp, "     rfpulse.dtycyc: %g\n", rfpulse.dtycyc);
  fprintf(fp, "     rfpulse.maxpw: %g\n", rfpulse.maxpw);
  fprintf(fp, "     rfpulse.num: %d\n", (int)rfpulse.num);
  fprintf(fp, "     rfpulse.max_b1: %g\n", rfpulse.max_b1);
  fprintf(fp, "     rfpulse.max_int_b1_sq: %g\n", rfpulse.max_int_b1_sq);
  fprintf(fp, "     rfpulse.max_rms_b1: %g\n", rfpulse.max_rms_b1);
  fprintf(fp, "     rfpulse.nom_fa: %g\n", rfpulse.nom_fa);
  fprintf(fp, "     rfpulse.act_fa: %g\n", *(rfpulse.act_fa));
  fprintf(fp, "     rfpulse.nom_pw: %g\n", rfpulse.nom_pw);
  fprintf(fp, "     rfpulse.nom_bw: %g\n", rfpulse.nom_bw);
  fprintf(fp, "     rfpulse.activity: %d\n", rfpulse.activity);
  fprintf(fp, "     rfpulse.isodelay: %d\n", rfpulse.isodelay);
  fprintf(fp, "     rfpulse.scale: %g\n", rfpulse.scale);
  fprintf(fp, "     rfpulse.res: %d\n", *(rfpulse.res));
  fprintf(fp, "     rfpulse.extgradfile: %d\n", rfpulse.extgradfile);

  fflush(fp);

}




void ks_print_rf(KS_RF r, FILE *fp) {
  if (r.rfpulse.activity == 0) {
    return;
  }

  fprintf(fp, "\nKS_RF (%s):\n", r.rfwave.description);
  fprintf(fp, "%s\n", r.designinfo);
  switch (r.role) {
  case KS_RF_ROLE_NOTSET: fprintf(fp, "role: KS_RF_ROLE_NOTSET\n"); break;
  case KS_RF_ROLE_EXC: fprintf(fp, "role: KS_RF_ROLE_EXC\n"); break;
  case KS_RF_ROLE_REF: fprintf(fp, "role: KS_RF_ROLE_REF\n"); break;
  case KS_RF_ROLE_CHEMSAT: fprintf(fp, "role: KS_RF_ROLE_CHEMSAT\n"); break;
  case KS_RF_ROLE_SPSAT: fprintf(fp, "role: KS_RF_ROLE_SPSAT\n"); break;
  case KS_RF_ROLE_INV: fprintf(fp, "role: KS_RF_ROLE_INV\n"); break;
  }
  fprintf(fp, "rfwave.res: %d\n", r.rfwave.res);
  fprintf(fp, "rfwave.duration: %d\n", r.rfwave.duration);
  fprintf(fp, "amp: %g\n", r.amp);
  fprintf(fp, "flip: %g\n", r.flip);
  fprintf(fp, "bw: %g\n", r.bw);
  fprintf(fp, "cf_offset: %g\n", r.cf_offset);
  fprintf(fp, "start2iso: %d\n", r.start2iso);
  fprintf(fp, "iso2end: %d\n", r.iso2end);

  ks_print_rfpulse(r.rfpulse, fp);

  if (r.rfwave.res == 0)
    fprintf(fp, "     rfwave: OFF\n");
  else
    fprintf(fp, "     rfwave: Waveform assigned\n");

  if (r.thetawave.res == 0)
    fprintf(fp, "     thetawave: OFF\n");
  else
    fprintf(fp, "     thetawave: Waveform assigned\n");

  fflush(fp);

} /* ks_print_rf */


void ks_print_selrf(KS_SELRF r, FILE *fp) {

  if (r.rf.rfpulse.activity == 0 || r.rf.rfwave.description == NULL || r.rf.rfwave.duration == 0) {
    return;
  }

  fprintf(fp, "\nKS_SELRF(%s):\n", r.rf.rfwave.description);

  fprintf(fp, "slthick: %g\n", r.slthick);
  if (r.rf.role == KS_RF_ROLE_REF)
    fprintf(fp, "crusher_dephasing: %g\n", r.crusher_dephasing);
  fprintf(fp, "bridge_crushers: %d\n", r.bridge_crushers);
  fprintf(fp, "pregrad_area_offset: %g\n", r.pregrad_area_offset);
  fprintf(fp, "postgrad_area_offset: %g\n", r.postgrad_area_offset);
  ks_print_trap(r.pregrad, fp);
  ks_print_trap(r.grad, fp);
  ks_print_trap(r.postgrad, fp);
  if (r.gradwave.res > 0) {
    fprintf(fp, "     gradwave: Waveform assigned\n");
  }
  fprintf(fp, "grad2rf_start: %d\n", r.grad2rf_start);
  fprintf(fp, "rf2grad_end: %d\n", r.rf2grad_end);

  ks_print_rf(r.rf, fp);

  fflush(fp);

} /* ks_print_selrf */




  STATUS ks_write_binary_file(void* data, const int size, const char* filename) {
    if (filename == NULL) {
      return KS_THROW("Null file name");
    }
    if (data == NULL) {
      return KS_THROW("Null data pointer");
    }
#if EPIC_RELEASE > 29
    psd::fileio::PsdPath psdpath;
    FILE* fp = fopen(psdpath.applicationDataPath(filename).c_str(), "wb");
#else
    FILE* fp = fopen(filename, "wb");
#endif

    if (fp == NULL) {
      return KS_THROW("Could not open file for writing: %s", filename);
    }

    fwrite(data, size, 1, fp);
    fclose(fp);

    return SUCCESS;
  }




STATUS ks_print_acqwaves(KS_SEQ_COLLECTION* seqcollection) {
  int seq;
  int resampler_index = 0;
  STATUS status = SUCCESS;

  char filename[512];
#ifdef PSD_HW /* on MR-scanner (host) */
  char outputdir_uid[512];
  char outputdir[512];
  char cmd[512];
  sprintf(outputdir_uid, "/usr/g/mrraw/kstmp/%010d/embed/", rhkacq_uid);
  sprintf(outputdir, "/usr/g/mrraw/kstmp/embed/");
  sprintf(filename, "%s/acq_waveforms.bin", outputdir_uid);
  sprintf(cmd, "mkdir -p %s > /dev/null", outputdir_uid);
  system(cmd);
  sprintf(cmd, "mkdir -p %s > /dev/null", outputdir);
  system(cmd);

#else /* e.g. WTools */
  sprintf(filename, "./acq_waveforms.bin");
#endif

#if EPIC_RELEASE > 29
  std::string acq_filename = "acq_waveforms.bin";
  psd::fileio::PsdPath psdpath;
  // applicationDataPath is PsdDynamicData in Orchestra, currently /opt/gehc/mr_psd_dist/root/var/psddata (and /srv/nfs/psd/var/psddata)
  FILE* fp = fopen(psdpath.applicationDataPath(acq_filename).c_str(), "w");
#else
  FILE* fp = fopen(filename, "wb");
#endif
  for(seq = 0; seq < seqcollection->numseq; seq++) {
    KS_GRADRFCTRL * gradrf = &seqcollection->seqctrlptr[seq]->gradrf;
    int acq;
    for(acq = 0; acq < gradrf->numacq; acq++) {
      KS_READ * read = gradrf->readptr[acq];
      KSCOMMON_RESAMPLER header = (KSCOMMON_RESAMPLER)KSCOMMON_INIT_RESAMPLER;
      switch (read->resampler.ndims) {
        case 3:
        {
          if (read->resampler.zwave) {
            header.zgrad.dwell = read->resampler.zwave->duration / read->resampler.zwave->res;
            header.zgrad.npts = read->duration / header.zgrad.dwell;
            header.zgrad.target_res = read->resampler.target_res;
          }
        }
        case 2:
        {
          if (read->resampler.ywave) {
            header.ygrad.dwell = read->resampler.ywave->duration / read->resampler.ywave->res;
            header.ygrad.npts = read->duration / header.ygrad.dwell;
            header.ygrad.target_res = read->resampler.target_res;
          }
        }
        case 1:
        {
          if (read->resampler.xwave) {
            header.xgrad.dwell = read->resampler.xwave->duration / read->resampler.xwave->res;
            header.xgrad.npts = read->duration / header.xgrad.dwell;
            header.xgrad.target_res = read->resampler.target_res;
          }

          header.acq_bw = read->filt.bw;
          header.acq_npts = read->filt.outputs;
          header.acq_tsp = read->filt.tsp;

          int state;
          for (state = 0; state < read->resampler.xwave->base.nstates; state++) {
            read->resampler.index[state] = resampler_index++;
            fwrite(&header, 1, sizeof(KSCOMMON_RESAMPLER), fp);
            if(header.xgrad.npts) {
              fwrite(&read->resampler.xwave->p_waveformstates[state][read->resampler.wave_st_idx], header.xgrad.npts, sizeof(float), fp);
            }
            if(header.ygrad.npts) {
              fwrite(&read->resampler.ywave->p_waveformstates[state][read->resampler.wave_st_idx], header.ygrad.npts, sizeof(float), fp);
            }
            if(header.zgrad.npts) {
              fwrite(&read->resampler.zwave->p_waveformstates[state][read->resampler.wave_st_idx], header.zgrad.npts, sizeof(float), fp);
            }
          }
          break;
        }
        case 0:
          break;
        default:
          return ks_error("%s - acq_resampling up to 3 dims supported (read.ndims = %d)", __FUNCTION__, read->resampler.ndims);
      }

    } /*gradrf*/

  } /*seqcollection*/
  fflush(fp);
  fclose(fp);
#ifdef PSD_HW /* on MR-scanner (host) */
 // Copy file to /usr/g/mrraw/kstmp/embed/
  sprintf(cmd, "cp %s %s", filename, outputdir);
  system(cmd);
#endif
  return status;
}




void ks_print_readwaves(KS_ECHOTRAIN* const echotrain, const char* suffix, int rhkacq_uid) {
  if (echotrain->numwaves == 0) {
    return;
  }

  char filename[512];
#ifdef PSD_HW /* on MR-scanner (host) */
  char outputdir_uid[512];
  char cmd[512];
  sprintf(outputdir_uid, "/usr/g/mrraw/kstmp/%010d/embed/", rhkacq_uid);
  sprintf(filename, "%s/readout_waveforms%s.bin", outputdir_uid, suffix);
  sprintf(cmd, "mkdir -p %s > /dev/null", outputdir_uid);
  system(cmd);
#else /* e.g. WTools */
  sprintf(filename, "./readout_waveforms%s.bin", suffix);
#endif

  FILE* fp = fopen(filename, "wb");
  int8_t wave_index;

  for (wave_index = 0; wave_index < echotrain->numwaves; wave_index++) {
    /* int object_index = echotrain->controls[wave_index].pg.object_index; */
    KS_READWAVE* const rw = &echotrain->readwaves[wave_index];
    const int16_t numplaced =ks_numplaced(&rw->grad.base);
    if (numplaced == 0) {
      ks_error("%s: %s has not been pg'd. Not included in %s", __FUNCTION__, rw->grad.description, filename);
      continue;
    }
    const int8_t acq_dwell = rw->acq.filt.tsp;
    const int8_t grad_dwell = rw->grad.duration / rw->grad.res;
    const uint16_t grad_samples_during_acq = rw->acq.duration / grad_dwell;
    const int8_t nstates = rw->grad.base.nstates;
    fwrite(&wave_index, sizeof(wave_index), 1, fp);
    fwrite(&nstates, sizeof(nstates), 1, fp);
    fwrite(&numplaced, sizeof(numplaced), 1, fp);
    fwrite(&grad_samples_during_acq, sizeof(grad_samples_during_acq),   1, fp);
    fwrite(&acq_dwell, sizeof(acq_dwell), 1, fp);
    fwrite(&grad_dwell, sizeof(grad_dwell), 1, fp);
    if (rw->grad.base.nstates > 1) {
      int state = 0;
      for (; state < rw->grad.base.nstates; state++) {
        fwrite(&(rw->grad.p_waveformstates[state][rw->acqdelay / grad_dwell]), sizeof(float), grad_samples_during_acq, fp);
      }
    } else {
      fwrite(&rw->grad.waveform[rw->acqdelay / grad_dwell], sizeof(float), grad_samples_during_acq, fp);
    }
  }
  fflush(fp);
  fclose(fp);
} /* ks_print_readwaves */


/*-*/


int ks_eval_clear_readwave(KS_READWAVE* readwave) {
  KS_READWAVE def_readwave;
  ks_init_readwave(&def_readwave);
  def_readwave.freqoffHz = readwave->freqoffHz;
  def_readwave.fov  = readwave->fov;
  def_readwave.res = readwave->res;
  *readwave = def_readwave;
  return SUCCESS;
}




float get_t1(float A, float s, float c) {
  /*#       ____________
  #      ╱          2 
  #  2⋅╲╱  2⋅A⋅s - c  
  #  ─────────────────
  #          s          */
  return (2 * sqrtf(2 * A * s - c*c)) / s;
}




float get_T(float A, float s, float c, float t1) {
/*#                  ________________________
  #                 ╱             2    2   2 
  #  2⋅c + s⋅t₁ + ╲╱  -8⋅A⋅s + 4⋅c  + s ⋅t₁  
  #  ────────────────────────────────────────
  #                    2⋅s                    */
  return (2 * c + s * t1 + sqrtf(-8.0f * A * s + 4 * c * c + s * s * t1 * t1)) / (2.0f * s);
}




float get_b(float A, float c, float T, float t1) {
/*# 2⋅A - c⋅t2
  # ───────────
  #     T        */
  float t2 = T - t1;
  return (2 * A - c * t2) / T;
}




typedef struct _ksramp_params {
  float slew1;
  float slew2;
  float t1;
  float t2;
} ks_rampparams_s;
#define KS_INIT_RAMPPARAMS {0, 0, 0, 0}

void get_ramp(ks_rampparams_s * out, float A, float s, float c) {
  float t1rnd = KS_RUP_GRD_FLOAT(get_t1(A, s, c));
  float Trnd = KS_RUP_GRD_FLOAT(get_T(A, s, c, t1rnd));
  float b = -1.0f;
  while (b < 0.0f) {
    b = get_b(A, c, Trnd, t1rnd);
    /* risk of violating maximum slew */
    Trnd -= GRAD_UPDATE_TIME;
  }
  out->t1 = t1rnd;
  out->slew1 = b / out->t1;
  out->t2 = Trnd - out->t1;
  out->slew2 = (c - b) / out->t2;

}

float ks_cycles_to_area(const float cycles, const float pixel_size_in_mm) {
  return 1.0e7 * cycles / (GAM * pixel_size_in_mm);
}


STATUS ks_area_to_amp(KS_WAVE * out, float area, float max_slew, float amp) {
  ks_rampparams_s params = KS_INIT_RAMPPARAMS;

  float minA = amp*amp / (2.0f*max_slew);
  if (area < minA) {
    return ks_error("%s: Minimum area is: %.2f", __FUNCTION__, minA);
  }

  /* gradient raster rounding can result in slew violations */
  float slew = max_slew;
  do {
    get_ramp(&params, area, slew, amp);
    /* Derate the max_slew and try again. */
    slew *= 0.999;
  } while(params.slew2 > max_slew);
  /* Create the wave object */
  {
    int t1grt = (int)(params.t1 / GRAD_UPDATE_TIME);
    int t2grt = (int)(params.t2 / GRAD_UPDATE_TIME);
    float s1grt = params.slew1 * GRAD_UPDATE_TIME;
    float s2grt = params.slew2 * GRAD_UPDATE_TIME;

    int i = 0;
    for(; i < t1grt; i++) out->waveform[i] = i * s1grt;
    for(; i < t2grt; i++) out->waveform[i] = i * s2grt;
    out->res = t1grt + t2grt;
    out->duration = out->res * GRAD_UPDATE_TIME;
  }

  return SUCCESS;

}




int ks_eval_append_blip(KS_WAVE* wave, const float max_s /* slewrate */, const int max_points, const float area) {
  float* prev = &wave->waveform[wave->res -1];
  float start_amp = *prev;

  /* Calculate tb slightly less than specified slewrate */
  const int tb = RUP_GRD(ceil(start_amp / max_s));
  const float sb = start_amp/tb;
  const float Mb = tb * start_amp / 2.0f;
  const float Ma = area - Mb;
  const int ta = RUP_GRD(ceil(( -start_amp + sqrt(start_amp*start_amp + max_s*Ma)) / max_s));
  if (ta < 0) {
    return ks_error("Ops");
  }
  const float sa = Ma/(ta*ta) - 2*start_amp/ta;

  const int num_a = ta / GRAD_UPDATE_TIME;
  const int num_b = tb / GRAD_UPDATE_TIME;
  if ((2*num_a + num_b) > max_points) {
    return ks_error("%s: num_a (%d) + num_b (%d) exceeds max points (%d)", __FUNCTION__, num_a, num_b, max_points);
  }
  int idx;
  float c_area = 0;
  for (idx = 0; idx < num_a; idx++) {
    float* cur = prev + 1;
    *cur = *prev + sa*GRAD_UPDATE_TIME;
    c_area += GRAD_UPDATE_TIME * (*prev + *cur) / 2.0f;
    prev++;
  }

  for (idx = 0; idx < num_a; idx++) {
    float* cur = prev + 1;
    *cur = *prev - sa*GRAD_UPDATE_TIME;
    c_area += GRAD_UPDATE_TIME * (*prev + *cur) / 2.0f;
    prev++;
  }
  for (idx = 0; idx < num_b; idx++) {
    float* cur = prev + 1;
    *cur = *prev - sb*GRAD_UPDATE_TIME;
    c_area += GRAD_UPDATE_TIME * (*prev + *cur) / 2.0f;
    prev++;
  }

  wave->res += 2*num_a + num_b;
  wave->duration = wave->res * GRAD_UPDATE_TIME;

  return SUCCESS;
}




int ks_file_exist(char *filename) {
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}




void ks_plot_host_slicetime_path(char* path) {
#ifdef PSD_HW /* on MR-scanner */
  sprintf(path, "/usr/g/mrraw/plot/%s/slicetime/", ks_psdname);
#else /* in Simulation */
  sprintf(path, "./plot/slicetime/");
#endif
}




void ks_plot_host_slicetime_fullfile(char* fullfile) {
  char path[250];
  ks_plot_host_slicetime_path(path);
  sprintf(fullfile, "%s%s_slicetime.json", path, ks_psdname);
}




int ks_plot_enable = 0;

void ks_plot_host_slicetime_delete() {
  char fname[500];
  ks_plot_host_slicetime_fullfile(fname);
  remove(fname);
  ks_plot_enable = 0;
}




void ks_plot_host_slicetime_begin() {
  if (ks_plot_filefmt == KS_PLOT_OFF) {
    return;
  }
  extern int optr;
  extern float pitscan;
  extern int opti;
  extern int opirprep;
  extern int opt1flair;
  extern int opt2flair;
  char path[250];
  char fname[500];
  char cmd[300];
  ks_plot_host_slicetime_path(path);
  ks_plot_host_slicetime_fullfile(fname);

  sprintf(cmd, "mkdir -p %s > /dev/null", path);
  system(cmd);
  /* Reset file */
  FILE* fp = fopen(fname, "w");
  fprintf(fp,
   "{\n"
   "\"metadata\": {\n"
   "\t\"psdname\": \"%s\",\n"
   "\t\"optr\": %d,\n"
   "\t\"opti\": %d,\n"
   "\t\"pitscan\": %.0f\n"
   "},\n"
   "\"passes\": [ {\n"
   "\t\"slicegroups\": [\n\t[{},\n\tP", /* P indicates a new pass has started */
   ks_psdname, optr, (opirprep || opt1flair || opt2flair) ? opti : 0, pitscan);
  fclose(fp);

  ks_plot_enable = 1;
}




void ks_plot_host_slicetime_endofslicegroup(const char* desc, const KS_PLOT_SLICEGROUP_MODE mode) {
  if (!ks_plot_enable || ks_plot_filefmt == KS_PLOT_OFF) {
    return;
  }
  char fname[500];
  ks_plot_host_slicetime_fullfile(fname);
  if (!ks_file_exist(fname)) {
    return;
  }
  char modestr[128];
  if (mode == KS_PLOT_SG_NOTSET) { strcpy(modestr,"KS_PLOT_SG_NOTSET"); }
  else if (mode == KS_PLOT_SG_ACQUISITION) { strcpy(modestr,"KS_PLOT_SG_ACQUISITION"); }
  else if (mode == KS_PLOT_SG_DUMMY) { strcpy(modestr,"KS_PLOT_SG_DUMMY"); }
  else if (mode == KS_PLOT_SG_CALIBRATION) { strcpy(modestr,"KS_PLOT_SG_CALIBRATION"); }

  FILE* fp = fopen(fname, "r+");
  if (fp == NULL) {return;}

  fseek(fp, -1, SEEK_END);
  int lastchar = fgetc(fp);
  if (lastchar == 'S') {
    fseek(fp, -1, SEEK_END); /* Expected */
  } else if (lastchar == 'G' || lastchar == 'P') {
    KS_THROW("%s is an empty slice group (lastchar \"%c\")", desc, lastchar);
    fclose(fp);
    return;
  } else {
    KS_THROW("last character \"%c\" not expected", lastchar);
  }


  fseek(fp, -1, SEEK_END);
  fprintf(fp,
          "{\n\t"
          "\"groupdescription\": \"%s\",\n\t"
          "\"mode\": \"%s\"\n\t"
          "}", desc == NULL ? "N/A" : desc,
          modestr);
  fputs("],[\n\t{},\n\tG", fp); /* G indicates slicegroup was closed */
  fflush(fp);
  fclose(fp);
}




void ks_plot_host_slicetime_endofpass(KS_PLOT_PASS_MODE pass_mode) {
  if (!ks_plot_enable || ks_plot_filefmt == KS_PLOT_OFF) {
    return;
  }
  static FILE *fp;
  char fname[500];
  ks_plot_host_slicetime_fullfile(fname);
  if (! ks_file_exist(fname)) {
    return;
  } 

  char passmode_str[50];
  switch (pass_mode) {
    case KS_PLOT_PASS_WAS_DUMMY: strcpy(passmode_str, "KS_PLOT_PASS_WAS_DUMMY"); break;
    case KS_PLOT_PASS_WAS_CALIBRATION: strcpy(passmode_str, "KS_PLOT_PASS_WAS_CALIBRATION"); break;
    case KS_PLOT_PASS_WAS_STANDARD: strcpy(passmode_str, "KS_PLOT_PASS_WAS_STANDARD"); break;
  }

  fp = fopen(fname, "r+");
  if (fp == NULL) {return;}

  fseek(fp, -1, SEEK_END);
  int lastchar = fgetc(fp);
  if (lastchar == 'S') {
    KS_THROW("Slice group will be automatically closed");
    ks_plot_host_slicetime_endofslicegroup("Auto closed", KS_PLOT_SG_NOTSET);
  } else if (lastchar == 'G') {
    /* Expected - closed slice group */
  } else if (lastchar == 'P') {
    KS_THROW("Trying to close an empty pass");
    fclose(fp);
    return;
  } else {
    KS_THROW("last character \"%c\" not expected", lastchar);
  }

  fseek(fp, -10, SEEK_END);
  fputs("],\n", fp);
  fprintf(fp,
            "\t\"passmode\": \"%s\"\n"
            "\t},{\n"
            "\t\"slicegroups\": [\n\t[{},\n\tP", passmode_str); /* P indicates a new pass or end is expected */
  fflush(fp);
  fclose(fp);
}




void ks_plot_host_slicetime(const KS_SEQ_CONTROL* ctrl,
                            int nslices,
                            float *slicepos_mm,
                            float slthick_mm,
                            KS_PLOT_EXCITATION_MODE excmode) {
  if (!ks_plot_enable || ks_plot_filefmt == KS_PLOT_OFF || ctrl->duration <= 0) {
    return;
  }
  static FILE *fp;
  int i;
  char excmode_str[50];
  char fname[1000];

  ks_plot_host_slicetime_fullfile(fname);
  if (! ks_file_exist(fname)) {
    return;
  }

  switch (excmode) {
  case KS_PLOT_STANDARD: strcpy(excmode_str, "KS_PLOT_STANDARD"); break;
  case KS_PLOT_NO_EXCITATION: strcpy(excmode_str, "KS_PLOT_NO_EXCITATION"); break;
  }

  if (ctrl->duration > 0) {
    fp = fopen(fname, "r+");
    if (fp == NULL) {return;}

    fseek(fp, -1, SEEK_END);
    int lastchar = fgetc(fp);
    if (!(lastchar == 'S' || lastchar == 'G' || lastchar == 'P')) {
      KS_THROW("%c not expected", lastchar);
    }
    fseek(fp, -1, SEEK_END);

    fprintf(fp,
            "{\n"
            "\t\t\"description\": \"%s\",\n"
            "\t\t\"excitationmode\": \"%s\",\n"
            "\t\t\"duration\": %0.3f,\n"
            "\t\t\"nseqinstances\": %d,\n"
            "\t\t\"minduration\": %0.3f,\n"
            "\t\t\"numrf\": %d,\n"
            "\t\t\"numtrap\": %d,\n"
            "\t\t\"numwave\": %d,\n"
            "\t\t\"numacq\": %d,\n"
            "\t\t\"momentstart\": %0.3f,\n"
            "\t\t\"slicethickness\": %0.3f,\n"
            "\t\t\"slicepos\": [",
            ctrl->description, excmode_str, ctrl->duration / 1000.0, ctrl->nseqinstances , ctrl->min_duration / 1000.0, ctrl->gradrf.numrf, ctrl->gradrf.numtrap, ctrl->gradrf.numwave, ctrl->gradrf.numacq, ctrl->momentstart /1000.0, slthick_mm);

    /* SMS slice locations */
    for (i = 0; i < (nslices - 1); i++) {
      fprintf(fp, "%0.3f, ", slicepos_mm == NULL ? 0.0 : slicepos_mm[i]);
    }
    fprintf(fp, "%0.3f]\n\t},S", slicepos_mm == NULL ? 0.0 : slicepos_mm[nslices - 1]); /* S indicates a slice was added */
    fflush(fp);
    fclose(fp);
  }

}




void ks_plot_host_slicetime_end() {
  if (!ks_plot_enable || ks_plot_filefmt == KS_PLOT_OFF) {
    ks_plot_enable = 0;
    return;
  }



  char outputdir_uid[250];
  char cmd[1000];

  char fname[500];
  ks_plot_host_slicetime_fullfile(fname);
  if (! ks_file_exist(fname)) {
    return;
  }

  FILE *fp = fopen(fname, "r+");
  int fd = fileno(fp);
  if (!fp) {
    return;
  }
  fseek(fp, -1, SEEK_END);
  int lastchar = fgetc(fp);
  if (lastchar == 'S') {
    KS_THROW("Slicegroup and pass will be closed automatically");
    ks_plot_host_slicetime_endofslicegroup("Auto closed", KS_PLOT_SG_NOTSET);
    ks_plot_host_slicetime_endofpass(KS_PLOT_PASS_WAS_STANDARD);
  } else if (lastchar == 'G') {
    /*KS_THROW("Pass will be closed automatically");*/
    ks_plot_host_slicetime_endofpass(KS_PLOT_PASS_WAS_STANDARD);
  } else if (lastchar == 'P') {
    /* Expected*/
  } else {
    KS_THROW("last character \"%c\" not expected", lastchar);
  }

  fseek(fp, -29, SEEK_END);
  fprintf(fp, "]\n}");
  fflush(fp);
  ftruncate(fd, ftell(fp));
  fclose(fp);

  outputdir_uid[0] = '\0';
#ifdef PSD_HW
  /* on MR-scanner */
  char outputdir[250];
  char pythonpath[] = "/usr/local/bin/apython";
  char psdplotpath[] = "/usr/local/bin/psdplot_slicetime.py";
  sprintf(outputdir, "/usr/g/mrraw/plot/%s/", ks_psdname);
  if (ks_plot_kstmp) {
    sprintf(outputdir_uid, "/usr/g/mrraw/kstmp/%010d/plot/", rhkacq_uid);
  }
#else
  /* in Simulation */
  char outputdir[] = "./plot/";
  char pythonpath[] = "apython";
  char psdplotpath[] = "../KSFoundation/psdplot/psdplot_slicetime.py";
#endif

    sprintf(cmd, "%s %s "
                 "%s "
                 "--outputdir %s %s &",
            pythonpath, psdplotpath, fname, outputdir, outputdir_uid);
    /* system(cmd); */

  ks_plot_enable = 0;
  return;
}




void ks_plot_host(const KS_SEQ_COLLECTION* seqcollection, const KS_PHASEENCODING_PLAN* plan) {
  int i;
  for (i = 0; i < seqcollection->numseq; i++) {
    ks_plot_host_seqctrl(seqcollection->seqctrlptr[i], plan);
  }
  return;
}




void ks_plot_host_seqctrl(const KS_SEQ_CONTROL *ctrl, const KS_PHASEENCODING_PLAN* plan) {
  ks_plot_host_seqctrl_manyplans(ctrl, &plan, 1);
}




void ks_plot_host_rtscales_amps(const KS_RT_SCALE_LOG *rtscalelog, FILE *fp, const int instance) {
  int shot;
  fputs("[", fp);
  if (rtscalelog->ampscales && (rtscalelog->num_completed_playouts * rtscalelog->num_instances) > 0) {
    int shots = rtscalelog->num_completed_playouts;
    for (shot = 0; shot < shots; shot++) {
      fprintf(fp, "%0.6f", ks_rt_scale_log_get(rtscalelog, instance, shot) );
      if (shot < (shots - 1)) {
        fputs(", ", fp);
      } else {
        fputs("],\n", fp);
      }
    }
  } else {
    fputs("1.0],\n", fp);
  }
}




void ks_plot_host_rtscales_states(const KS_RT_SCALE_LOG *rtscalelog, FILE *fp, const int instance) {
  int shot;
  fputs("[", fp);
  if (rtscalelog->states && (rtscalelog->num_completed_playouts * rtscalelog->num_instances) > 0) {
    int shots = rtscalelog->num_completed_playouts;
    for (shot = 0; shot < shots; shot++) {
      fprintf(fp, "%d", ks_rt_scale_log_get_state(rtscalelog, instance, shot) );
      if (shot < (shots - 1)) {
        fputs(", ", fp);
      } else {
        fputs("],\n", fp);
      }
    }
  } else {
    fputs("0],\n", fp);
  }
}




void ks_plot_host_wavestates(const KS_WAVE* waveptr, FILE* fp) {
    if (waveptr->base.nstates) {
    int state = 0;
    int w;
    fprintf(fp, "\t\t\"states\": [\n");
    for (; state < waveptr->base.nstates; state++) {
      fprintf(fp, "\t\t\t [0.0,");
      for (w = 0; w < waveptr->res; w++) {
        fprintf(fp, "%0.6f,", waveptr->p_waveformstates[state][w]);
      }

      if (state < (waveptr->base.nstates - 1)) {
        fprintf(fp, "0.0],\n");
      } else {
        fprintf(fp, "0.0]\n");
      }
    }
    fprintf(fp, "\t\t],\n");
  }
}




void ks_plot_write_peplans(const KS_PHASEENCODING_PLAN* const * plans, const int num_plans, FILE* fp) {

  if (plans == NULL || num_plans <= 0) {
        return;
  }
  int plan_idx = 0;
  fprintf(fp, "\"phaseencplans\": [\n");
  for (; plan_idx < num_plans; plan_idx++) {
    const KS_PHASEENCODING_PLAN* const plan = plans[plan_idx];
    if (plan) {
      fprintf(fp, "{\n");
      fprintf(fp, "\t\"description\": \"%s\",\n", plan->description);
      fprintf(fp, "\t\"num_shots\": %d,\n", plan->num_shots);
      fprintf(fp, "\t\"coord_type\": [\"%s\", \"%s\"],\n",  plan->coord_type[0] == CARTESIAN_COORD ? "CARTESIAN_COORD" : "RADIAL_COORD",
                                                            plan->coord_type[1] == CARTESIAN_COORD ? "CARTESIAN_COORD" : "RADIAL_COORD");
      fprintf(fp, "\t\"encodes_per_shot\": %d,\n", plan->encodes_per_shot);
      fprintf(fp, "\t\"entries\": [\n");
      int shot;
      for (shot = 0; shot < plan->num_shots; shot++) {
        fprintf(fp, "\t\t[");
        int encode;
        for (encode = 0; encode < plan->encodes_per_shot; encode++) {
          const KS_PHASEENCODING_COORD coord = ks_phaseencoding_get(plan, encode, shot);
          fprintf(fp, "[%d, %d]", coord.ky, coord.kz);
          if (encode < (plan->encodes_per_shot - 1)) {
            fprintf(fp, ", ");
          }
        }
        fprintf(fp, "]");
        if (shot < (plan->num_shots - 1)) {
          fprintf(fp, ",\n");
        }
      }
      fprintf(fp, "]\n");

      if (plan_idx < (num_plans-1)) {
        fprintf(fp, "},\n");
      } else {
        fprintf(fp, "}\n");
      }
    } else {
      /* KS_THROW("plans[%d] is NULL", plan_idx); */
    }
  }
  fputs("]", fp);

  return;
}




void ks_plot_host_seqctrl_manyplans(const KS_SEQ_CONTROL *ctrl, const KS_PHASEENCODING_PLAN** plans, const int num_plans) {
#if defined (PSD_HW) && defined (IPG)
  /* on IPG and on scanner, i.e. we are actually scanning so no plots for you */
  return;
#endif

  if (ks_plot_filefmt == KS_PLOT_OFF || ctrl->duration == 0 || ctrl->nseqinstances ==  0) {
    return;
  }

  char boardc[9] = "xyzrRtos"; /* x,y,z,rho1, (unused rho2), theta, omega, ssp */
  char fname[1000]; /* Full filename of the json file */
  char path[250]; /* Directory of json file */
  char outputdir[250]; /* Where plots are saved */
  char outputdir_uid[250]; /* rhkacq_uid based output directory (for automatic transfers) */

  ks_plot_filename(fname, path, outputdir, outputdir_uid, ctrl->description);

  char cmd[250];
  FILE* fp;
  int i, j;
  KS_SEQLOC loc;

#ifdef PSD_HW
  /* on MR-scanner */
  if (ks_plot_kstmp) {
    sprintf(cmd, "mkdir -p %s > /dev/null", outputdir_uid);
    system(cmd);
  }
#endif
  sprintf(cmd, "mkdir -p %s > /dev/null", path);
  system(cmd);

  fp = fopen(fname, "w");


  fprintf(fp, "{\n");
  fprintf(fp, "\"metadata\": {\n");
  fprintf(fp, "\t\"mode\": \"host\",\n");
  fprintf(fp, "\t\"psdname\":  \"%s\",\n", ks_psdname);
  fprintf(fp, "\t\"sequence\": \"%s\",\n", ctrl->description);
  fprintf(fp, "\t\"duration\": %0.3f,\n", ctrl->duration/1000.0);
  fprintf(fp, "\t\"min_duration\": %0.3f,\n", ctrl->min_duration/1000.0);
  fprintf(fp, "\t\"ssi_time\": %0.3f,\n", ctrl->ssi_time/1000.0);
  fprintf(fp, "\t\"momentstart\": %.3f,\n", ctrl->gradient_momentstart/1000.0);
  fprintf(fp, "\t\"optr_desc\": \"%s\"\n", _optr.descr);
  fprintf(fp, "},\n");

  /* PHASE ENCODING PLANS */
  ks_plot_write_peplans( plans,  num_plans, fp);

  fputs(",\n", fp);
  /* TRAPEZOIDS */
  fprintf(fp, "\"frames\": [\n");
  fprintf(fp, "{},\n");
  fprintf(fp, "{\n\"trapz\": {\n");
  for (i = 0; i < ctrl->gradrf.numtrap; i++) { /* each unique trap object */
    KS_TRAP* trap = ctrl->gradrf.trapptr[i];
    fprintf(fp, "\t\"%s\": {\n", trap->description);
    fprintf(fp, "\t\t\"ramptime\": %0.3f,\n", trap->ramptime / 1000.0);
    fprintf(fp, "\t\t\"plateautime\": %0.3f,\n", trap->plateautime / 1000.0);
    fprintf(fp, "\t\t\"duration\": %0.3f,\n", trap->duration / 1000.0);
    fprintf(fp, "\t\t\"amp\": %0.6f,\n", isnan(trap->amp) ? 0.0f : trap->amp);
    fprintf(fp, "\t\t\"instances\": [");

    for (j = 0; j < trap->base.ninst; j++) { /* each instance of the trap */
      fprintf(fp, "{\n");
      loc = trap->locs[j];
      fprintf(fp, "\t\t\t\"board\": \"%c\",\n", boardc[loc.board]);
      fprintf(fp,"\t\t\t\"time\": %0.3f,\n", loc.pos / 1000.0);
      fputs("\t\t\t\"rtscale\": ", fp);
      ks_plot_host_rtscales_amps(&trap->rtscaling, fp, j);
      fputs("\t\t\t\"state\": ", fp);
      ks_plot_host_rtscales_states(&trap->rtscaling, fp, j);
      fprintf(fp,"\t\t\t\"ampscale\": %0.6f\n", loc.ampscale);
      if (j < (trap->base.ninst - 1)) {
        fprintf(fp, "\t\t},");
      } else {
        fprintf(fp, "\t\t}");    
      }
    }
    fprintf(fp, "]\n");

    if (i < (ctrl->gradrf.numtrap - 1)) {
      fprintf(fp, "\t},\n");
    } else {
      fprintf(fp, "\t}\n");
    }
  }
  fprintf(fp, "},\n");

  /* ACQUISITIONS */
  fprintf(fp, "\"acquisitions\": [\n");
  for (i = 0; i < ctrl->gradrf.numacq; i++) { /* Each unique KS_READ object */
    KS_READ* acq = ctrl->gradrf.readptr[i];
    fprintf(fp, "\t{\n");
    fprintf(fp, "\t\t\"description\": \"%s\",\n", acq->description);
    fprintf(fp, "\t\t\"duration\": %0.3f,\n", acq->duration / 1000.0);
    fprintf(fp, "\t\t\"rbw\": %0.3f,\n", acq->rbw);
    fprintf(fp, "\t\t\"samples\": %d,\n", acq->filt.outputs);
    fprintf(fp, "\t\t\"time\": [");
    for (j = 0; j < acq->base.ninst; ++j) {
      fprintf(fp, "%0.3f", acq->pos[j] / 1000.0);
      if (j < (acq->base.ninst - 1)) {
        fprintf(fp, ", ");
      }
    }
    fprintf(fp, "]\n");
    if (i < (ctrl->gradrf.numacq - 1)) {
      fprintf(fp, "\t},\n");
    } else {
      fprintf(fp, "\t}\n");
    }
  }
  fprintf(fp, "],\n");

  /* RF (RHO1) */
  fprintf(fp, "\"rf\": {\n");
  for (i = 0; i < ctrl->gradrf.numrf; i++) { /* each unique RF object */
    KS_RF* rfptr = ctrl->gradrf.rfptr[i];
    if (rfptr->rfwave.res > 0) {
      char roledesc[50];
      if (rfptr->role == KS_RF_ROLE_EXC) {
        strcpy(roledesc,"KS_RF_ROLE_EXC");
      } else if (rfptr->role ==  KS_RF_ROLE_REF) {
        strcpy(roledesc,"KS_RF_ROLE_REF");
      } else if (rfptr->role ==  KS_RF_ROLE_SPSAT) {
        strcpy(roledesc,"KS_RF_ROLE_SPSAT");
      } else if (rfptr->role == KS_RF_ROLE_CHEMSAT) {
        strcpy(roledesc, "KS_RF_ROLE_CHEMSAT");
      } else if (rfptr->role == KS_RF_ROLE_INV) {
        strcpy(roledesc, "KS_RF_ROLE_INV");
      } else if (rfptr->role == KS_RF_ROLE_SPSAT) {
        strcpy(roledesc, "KS_RF_ROLE_SPSAT");
      } else {
        strcpy(roledesc,"NONE");
      }
      fprintf(fp, "\t\"%s\": {\n", rfptr->rfwave.description);
      fprintf(fp, "\t\t\"description\": \"%s\",\n", rfptr->rfwave.description);
      fprintf(fp, "\t\t\"flipangle\": %0.3f,\n", rfptr->flip);
      fprintf(fp, "\t\t\"nominal_flipangle\": %0.3f,\n", rfptr->rfpulse.nom_fa);
      fprintf(fp, "\t\t\"max_b1\": %0.6f,\n", rfptr->rfpulse.max_b1);
      fprintf(fp, "\t\t\"amp\": %0.6f,\n", rfptr->amp);
      fprintf(fp, "\t\t\"isodelay\": %0.3f,\n", (rfptr->iso2end_subpulse > 0 ? rfptr->rfwave.duration - rfptr->iso2end_subpulse : rfptr->start2iso) / 1000.0);
      fprintf(fp, "\t\t\"iso2end_subpulse\": %0.3f,\n", rfptr->iso2end_subpulse / 1000.0);
      fprintf(fp, "\t\t\"duration\": %0.3f,\n", rfptr->rfwave.duration / 1000.0);
      fprintf(fp, "\t\t\"role\": \"%s\",\n", roledesc);

      fprintf(fp, "\t\t\"omega\": {\n");
      if (rfptr->omegawave.res > 0) {
        fprintf(fp, "\t\t\t\"description\": \"%s\",\n", rfptr->omegawave.description);
        fprintf(fp, "\t\t\t\"duration\": %0.3f,\n", rfptr->omegawave.duration / 1000.0);
        ks_plot_host_wavestates(&rfptr->omegawave, fp);
        fprintf(fp, "\t\t\t\"instances\": [");
        for (j = 0; j < rfptr->omegawave.base.ninst; j++) {
          fprintf(fp, "{\n");
          loc = rfptr->omegawave.locs[j];
          fprintf(fp, "\t\t\t\t\"time\": %0.3f,\n", loc.pos/1000.0);
          fprintf(fp, "\t\t\t\t\"board\": \"%c\",\n", boardc[loc.board]);
          fputs("\t\t\t\t\"rtscale\": ", fp);
          ks_plot_host_rtscales_amps(&rfptr->omegawave.rtscaling, fp, j);
          fputs("\t\t\t\"state\": ", fp);
          ks_plot_host_rtscales_states(&rfptr->omegawave.rtscaling, fp, j);
          fprintf(fp, "\t\t\t\t\"ampscale\": %0.6f\n", loc.ampscale);
          if (j < (rfptr->omegawave.base.ninst - 1)) {
            fprintf(fp, "\t\t\t},");
          } else {
            fprintf(fp, "\t\t\t}");
          }
        }
        fprintf(fp, "]\n");
      }
      fprintf(fp, "\t\t},\n"); /* Omega */

      fprintf(fp, "\t\t\"theta\": {\n");
      if (rfptr->thetawave.res > 0) {
        fprintf(fp, "\t\t\t\"description\": \"%s\",\n", rfptr->thetawave.description);
        fprintf(fp, "\t\t\t\"duration\": %0.3f,\n", rfptr->thetawave.duration / 1000.0);

        ks_plot_host_wavestates(&rfptr->thetawave, fp);
        fprintf(fp, "\t\t\t\"instances\": [");
        for (j = 0; j < rfptr->thetawave.base.ninst; j++) {
          fprintf(fp, "{\n");
          loc = rfptr->thetawave.locs[j];
          fprintf(fp, "\t\t\t\t\"time\": %0.3f,\n", loc.pos/1000.0);
          fprintf(fp, "\t\t\t\t\"board\": \"%c\",\n", boardc[loc.board]);
          fputs("\t\t\t\t\"rtscale\": ", fp);
          ks_plot_host_rtscales_amps(&rfptr->thetawave.rtscaling, fp, j);
          fputs("\t\t\t\t\"state\": ", fp);
          ks_plot_host_rtscales_states(&rfptr->thetawave.rtscaling, fp, j);
          fprintf(fp, "\t\t\t\t\"ampscale\": %0.6f\n", loc.ampscale);
          if (j < (rfptr->thetawave.base.ninst - 1)) {
            fprintf(fp, "\t\t\t},");
          } else {
            fprintf(fp, "\t\t\t}");
          }
        }
        fprintf(fp, "]\n");
      }
      fprintf(fp, "\t\t},\n");  /* Theta */


      fflush(fp);
      ks_plot_host_wavestates(&rfptr->rfwave, fp);
      fflush(fp);
      fprintf(fp, "\t\t\"instances\": [");
      for (j = 0; j < rfptr->rfwave.base.ninst; j++) { /* each instance of the wave (RHO1) */
        fprintf(fp, "{\n");
        loc = rfptr->rfwave.locs[j];
        fprintf(fp, "\t\t\t\"time\": %0.3f,\n", loc.pos/1000.0);
        fprintf(fp, "\t\t\t\"board\": \"%c\",\n", boardc[loc.board]);
        fputs("\t\t\t\"rtscale\": ", fp);
        ks_plot_host_rtscales_amps(&rfptr->rfwave.rtscaling, fp, j);
        fputs("\t\t\t\"state\": ", fp);
        ks_plot_host_rtscales_states(&rfptr->rfwave.rtscaling, fp, j);
        fprintf(fp, "\t\t\t\"ampscale\": %0.6f\n", loc.ampscale);
        if (j < (rfptr->rfwave.base.ninst - 1)) {
          fprintf(fp, "\t\t},");
        } else {
          fprintf(fp, "\t\t}");
        }
      }
      fprintf(fp, "]\n");

      if (i < (ctrl->gradrf.numrf - 1)) {
        fprintf(fp, "\t},\n");
      } else {
        fprintf(fp, "\t}\n");
      }
    } /* res > 0 */
  } /* rf */
  fprintf(fp, "},\n");

  /* Wait pulses */
  fputs("\"waits\": {\n",fp);
  for (i = 0; i < ctrl->gradrf.numwait; i++) {
    KS_WAIT* waitptr = ctrl->gradrf.waitptr[i];
    if (waitptr->max_duration > 0) {
      fprintf(fp, "\t\"%s\": {\n", waitptr->description);
      fprintf(fp, "\t\t\"description\": \"%s\",\n", waitptr->description);
      fprintf(fp, "\t\t\"max_duration\": %0.3f,\n", waitptr->max_duration / 1000.0);
      fprintf(fp, "\t\t\"pg_duration\": %0.3f,\n", waitptr->pg_duration / 1000.0);
      fprintf(fp, "\t\t\"instances\": [");
      for (j = 0; j < waitptr->base.ninst; j++) { /* each instance of the wave */
        fprintf(fp, "{\n");
        loc = waitptr->locs[j];
        fprintf(fp, "\t\t\t\"board\": \"%c\",\n", boardc[loc.board]);
        fputs("\t\t\t\"duration\": ", fp);
        if (waitptr->rtscaling.states && (waitptr->rtscaling.num_completed_playouts * waitptr->rtscaling.num_instances) > 0) {
          ks_plot_host_rtscales_states(&waitptr->rtscaling, fp, j);
        } else { /* Default to pg duration if not set in scan */
          fprintf(fp, "[%d],\n", waitptr->pg_duration);
        }
        fprintf(fp, "\t\t\t\"time\": %0.3f\n", loc.pos/1000.0);
        if (j < (waitptr->base.ninst - 1)) {
          fprintf(fp, "\t\t},");
        } else {
          fprintf(fp, "\t\t}");
        }
      }
      fprintf(fp, "]\n");
      if (i < (ctrl->gradrf.numwait - 1)) {
        fprintf(fp, "\t},\n");
      } else {
        fprintf(fp, "\t}\n");
      }
    }
  }
  fputs("},\n", fp); /* End waits */

  fprintf(fp, "\"waves\": {\n");
  /* other (non-RF) waves */
  for (i = 0; i < ctrl->gradrf.numwave; i++) {
    KS_WAVE* waveptr = ctrl->gradrf.waveptr[i];
    if (waveptr->res > 0) {
      fprintf(fp, "\t\"%s\": {\n", waveptr->description);
      fprintf(fp, "\t\t\"description\": \"%s\",\n", waveptr->description);
      fprintf(fp, "\t\t\"duration\": %0.3f,\n", waveptr->duration / 1000.0);
      ks_plot_host_wavestates(waveptr, fp);
      fprintf(fp, "\t\t\"instances\": [");
      for (j = 0; j < waveptr->base.ninst; j++) { /* each instance of the wave */
        fprintf(fp, "{\n");
        loc = waveptr->locs[j];
        fprintf(fp, "\t\t\t\"board\": \"%c\",\n", boardc[loc.board]);
        fprintf(fp, "\t\t\t\"time\": %0.3f,\n", loc.pos/1000.0);
        fputs("\t\t\t\"rtscale\": ", fp);
        ks_plot_host_rtscales_amps(&waveptr->rtscaling, fp, j);
        fputs("\t\t\t\"state\": ", fp);
        ks_plot_host_rtscales_states(&waveptr->rtscaling, fp, j);
        fprintf(fp, "\t\t\t\"ampscale\": %0.6f\n", loc.ampscale);
        if (j < (waveptr->base.ninst - 1)) {
          fprintf(fp, "\t\t},");
        } else {
          fprintf(fp, "\t\t}");
        }
      }
      fprintf(fp, "]\n");
      if (i < (ctrl->gradrf.numwave - 1)) {
        fprintf(fp, "\t},\n");
      } else {
        fprintf(fp, "\t}\n");
      }
    } /* res > 0 */
  } 
  fputs("}\n}\n]\n}", fp); /* end waves, frame, frames, end */
  fflush(fp);
  fclose(fp);

  return;
}




void ks_plot_filename(char* fname, char* path, char* outputdir, char* outputdir_uid, const char * const seq_desc) {
#if defined (PSD_HW) && defined (IPG)
  return;
#endif


#ifdef PSD_HW
  /* on MR-scanner */
  sprintf(path,  "/usr/g/mrraw/plot/%s/host/", ks_psdname);
  if (outputdir) {
   sprintf(outputdir, "/usr/g/mrraw/plot/%s/", ks_psdname); 
  }

  if (outputdir_uid && ks_plot_kstmp) {
    sprintf(outputdir_uid, "/usr/g/mrraw/kstmp/%010d/plot/", rhkacq_uid);
  }
#else
  /* in Simulation */
  sprintf(path, "./plot/host/");
  if (outputdir) {
   sprintf(outputdir, "./plot/"); 
  }
#endif

  sprintf(fname, "%spsdplot_%s_%s.json", path, ks_psdname, seq_desc);

return;
}




void ks_plot_saveconfig(KS_SEQ_CONTROL*  ctrl) {
#ifdef IPG
  return;
#endif
  char fname[1000]; /* Full filename of the json file */
  char path[250]; /* Directory of json file */
  ks_plot_filename(fname, path, NULL, NULL, ctrl->description);
  int i, j;
  FILE* fp;
  if ((fp = fopen(fname, "r+"))) {
    ks_dbg("The file %s exists", fname);
  } else {
    ks_dbg("The file %s is not there", fname);
    return;
  }
  fseek(fp, -4, SEEK_END);
  fputs(",\n"
        "\t{\n", fp);
  for (i = 0; i < ctrl->gradrf.numtrap; i++) {
    KS_TRAP* trap = ctrl->gradrf.trapptr[i];
    fprintf(fp, "\t\t\"%s\": [", trap->description);

    for (j = 0; j < ks_numplaced(&trap->base); j++) { /* each instance of the trap */
      fprintf(fp, "%0.6f", ks_rt_scale_log_get(&trap->rtscaling, j, ctrl->current_playout));
      if (j < (ks_numplaced(&trap->base) - 1)) {
        fputs(", ", fp);
      }
    }
    fputs("]", fp);
     if (i == (ctrl->gradrf.numtrap - 1)) {
      fputs("\n", fp);
    } else {
      fputs(",\n", fp);
    } 
  }

  fputs("\t}", fp);


  fputs("\n"
        "]\n"
        "}",fp);
  fflush(fp);
  fclose(fp);

}




STATUS ks_repeat_peplan(KS_PHASEENCODING_PLAN* peplan, const KS_PHASEENCODING_REPEAT_DESIGN * const repeat) {
  if (repeat->num_repeats < 2 || repeat->mode == NO_REPEAT) {
    return SUCCESS;
  }

  /* Figure out size of new plan */
  int new_encodes_per_shot = peplan->encodes_per_shot;
  int new_num_shots = peplan->num_shots;
  switch (repeat->mode) {
  case INTERLEAVED_LONGER_SHOTS:
  case SEQUENTIAL_LONGER_SHOTS: {
    new_encodes_per_shot *= repeat->num_repeats;
    break;
  }
  case INTERLEAVED_EXTRA_SHOTS:
  case SEQUENTIAL_EXTRA_SHOTS: {
    new_num_shots *= repeat->num_repeats;
    break;
  }
  default:
    return KS_THROW("Repeat mode (%d) not supported", (int)repeat->mode);
    break;
  }

  /* Allocate new plan */
  KS_PHASEENCODING_PLAN new_peplan = *peplan;
  new_peplan.encodes_per_shot = new_encodes_per_shot;
  new_peplan.num_shots = new_num_shots;
  STATUS status = ks_phaseencoding_alloc(&new_peplan, new_encodes_per_shot, new_num_shots);
  KS_RAISE(status);

  /* Copy according to mode */
  int shot, encode, rep;
  for (shot = 0; shot < peplan->num_shots; shot++) {
    for (encode = 0; encode < peplan->encodes_per_shot; encode++) {
      KS_PHASEENCODING_COORD coord = ks_phaseencoding_get(peplan, encode, shot);

      int encode_index = encode;
      int shot_index = shot;
      for (rep = 0; rep < repeat->num_repeats; rep++) {
        switch (repeat->mode) {
          case INTERLEAVED_LONGER_SHOTS: {
            encode_index = encode * repeat->num_repeats + rep;
            break;
          } case SEQUENTIAL_LONGER_SHOTS: {
            encode_index = encode + peplan->encodes_per_shot * rep;
            break;
          } case INTERLEAVED_EXTRA_SHOTS: {
            shot_index = shot * repeat->num_repeats + rep;
            break;
          } case SEQUENTIAL_EXTRA_SHOTS: {
            shot_index = shot + peplan->num_shots * rep;
            break;
          } default: {
            return KS_THROW("Repeat mode (%d) not implemented", (int)repeat->mode);
            break;
          }
        }

        ks_phaseencoding_set(&new_peplan, encode_index, shot_index, coord.ky, coord.kz);
      }
    }
  }
  *peplan = new_peplan;
  return SUCCESS;
}




typedef int (*ks_comparator)(const void*, const void*);

int ks_comp_kcoord_r_t(const void* a, const void* b) {
    KS_KCOORD* k_a =(KS_KCOORD*)a;
    KS_KCOORD* k_b =(KS_KCOORD*)b;
    if (k_a->r > k_b->r) {
      return 1;
    } else if (k_a->r < k_b->r) {
      return -1;
    }

    if (k_a->t > k_b->t) {
      return 1;
    } else if (k_a->t < k_b->t) {
      return -1;
    }

    return 0;
}

int ks_comp_kcoord_t_r(const void* a, const void* b) {
    KS_KCOORD* k_a =(KS_KCOORD*)a;
    KS_KCOORD* k_b =(KS_KCOORD*)b;
    if (k_a->t > k_b->t) {
      return 1;
    } else if (k_a->t < k_b->t) {
      return -1;
    }

    if (k_a->r > k_b->r) {
      return 1;
    } else if (k_a->r < k_b->r) {
      return -1;
    }

    return 0;
}




int ks_comp_kcoord_dist_z_y(const void* a, const void* b) {
    KS_KCOORD* k_a = (KS_KCOORD*)a;
    KS_KCOORD* k_b = (KS_KCOORD*)b;

    const float z_distance = fabs(k_a->z + 0.5) - fabs(k_b->z + 0.5);
    if (!areWithinTol(z_distance, 0.0, 0.1)) {
      return (z_distance > 0.0 ? 1 : 0);
    }

    const float y_distance = fabs(k_a->y + 0.5) - fabs(k_b->y + 0.5);
    if (!areWithinTol(y_distance, 0.0, 0.1)) {
      return (y_distance > 0.0 ? 1 : 0);
    }

    return 0;
}




int ks_comp_kview_dist_z_y(const void* a, const void* b) {
    KS_VIEW* view_a =(KS_VIEW*)a;
    KS_VIEW* view_b =(KS_VIEW*)b;
    return ks_comp_kcoord_dist_z_y(view_a->coord, view_b->coord);
}




int ks_comp_kcoord_dist_y_z(const void* a, const void* b) {
    KS_KCOORD* k_a = (KS_KCOORD*)a;
    KS_KCOORD* k_b = (KS_KCOORD*)b;
    const float y_distance = fabs(k_a->y + 0.5) - fabs(k_b->y + 0.5);
    if (!areWithinTol(y_distance, 0.0, 0.1)) {
      return (y_distance > 0.0 ? 1 : 0);
    }

    const float z_distance = fabs(k_a->z + 0.5) - fabs(k_b->z + 0.5);
    if (!areWithinTol(z_distance, 0.0, 0.1)) {
      return (z_distance > 0.0 ? 1 : 0);
    }

    return 0;
}




int ks_comp_kview_dist_y_z(const void* a, const void* b) {
    KS_VIEW* view_a =(KS_VIEW*)a;
    KS_VIEW* view_b =(KS_VIEW*)b;
    return ks_comp_kcoord_dist_y_z(view_a->coord, view_b->coord);
}




int ks_comp_kcoord_z_y(const void* a, const void* b) {
    KS_KCOORD* k_a =(KS_KCOORD*)a;
    KS_KCOORD* k_b =(KS_KCOORD*)b;
    if (k_a->z > k_b->z) {
      return 1;
    } else if (k_a->z < k_b->z) {
      return -1;
    }

    if (k_a->y > k_b->y) {
      return 1;
    } else if (k_a->y < k_b->y) {
      return -1;
    }

    return 0;
}




int ks_comp_kcoord_y_z(const void* a, const void* b) {
    KS_KCOORD* k_a =(KS_KCOORD*)a;
    KS_KCOORD* k_b =(KS_KCOORD*)b;
    if (k_a->y > k_b->y) {
      return 1;
    } else if (k_a->y < k_b->y) {
      return -1;
    }

    if (k_a->z > k_b->z) {
      return 1;
    } else if (k_a->z < k_b->z) {
      return -1;
    }

    return 0;
}




int ks_comp_kview_r_t(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;
    if (k_a->coord->r > k_b->coord->r) {
      return 1;
    } else if (k_a->coord->r < k_b->coord->r) {
      return -1;
    }

    if (k_a->coord->t > k_b->coord->t) {
      return 1;
    } else if (k_a->coord->t < k_b->coord->t) {
      return -1;
    }

    return 0;
}




int ks_comp_kview_t_r(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;

    if (k_a->coord->t > k_b->coord->t) {
      return 1;
    } else if (k_a->coord->t < k_b->coord->t) {
      return -1;
    }

    if (k_a->coord->r > k_b->coord->r) {
      return 1;
    } else if (k_a->coord->r < k_b->coord->r) {
      return -1;
    }

    return 0;
}




int ks_comp_kview_y_z(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;
    if (k_a->coord->y > k_b->coord->y) {
      return 1;
    } else if (k_a->coord->y < k_b->coord->y) {
      return -1;
    }

    if (k_a->coord->z > k_b->coord->z) {
      return 1;
    } else if (k_a->coord->z < k_b->coord->z) {
      return -1;
    }
    return 0;
}




int ks_comp_kview_z_y(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;
    if (k_a->coord->z > k_b->coord->z) {
      return 1;
    } else if (k_a->coord->z < k_b->coord->z) {
      return -1;
    }

    if (k_a->coord->y > k_b->coord->y) {
      return 1;
    } else if (k_a->coord->y < k_b->coord->y) {
      return -1;
    }
    return 0;
}




int ks_comp_kview_y_r(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;
    if (k_a->coord->y > k_b->coord->y) {
      return 1;
    } else if (k_a->coord->y < k_b->coord->y) {
      return -1;
    }

    if (k_a->coord->r > k_b->coord->r) {
      return 1;
    } else if (k_a->coord->r < k_b->coord->r) {
      return -1;
    }
    return 0;
}




int ks_comp_kview_z_r(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;
    if (k_a->coord->z > k_b->coord->z) {
      return 1;
    } else if (k_a->coord->z < k_b->coord->z) {
      return -1;
    }

    if (k_a->coord->r > k_b->coord->r) {
      return 1;
    } else if (k_a->coord->r < k_b->coord->r) {
      return -1;
    }
    return 0;
}

int ks_comp_kview_user1(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;
    if (k_a->coord->user1 > k_b->coord->user1) {
      return 1;
    } else if (k_a->coord->user1 < k_b->coord->user1) {
      return -1;
    }
    return 0;
}



int ks_comp_kview_e_tr(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;

    if (k_a->encode > k_b->encode) {
      return 1;
    } else if (k_a->encode < k_b->encode) {
      return -1;
    }

    return ks_comp_kview_t_r(a, b);

}




int ks_comp_kview_e_rt(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;

    if (k_a->encode > k_b->encode) {
      return 1;
    } else if (k_a->encode < k_b->encode) {
      return -1;
    }

    return ks_comp_kview_r_t(a, b);

}




int ks_comp_kview_e_zy(const void* a, const void* b) {
  KS_VIEW* k_a =(KS_VIEW*)a;
  KS_VIEW* k_b =(KS_VIEW*)b;

  if (k_a->encode > k_b->encode) {
    return 1;
  } else if (k_a->encode < k_b->encode) {
    return -1;
  }

  return ks_comp_kview_z_y(a, b);

}




int ks_comp_kview_e_yz(const void* a, const void* b) {
  KS_VIEW* k_a =(KS_VIEW*)a;
  KS_VIEW* k_b =(KS_VIEW*)b;

  if (k_a->encode > k_b->encode) {
    return 1;
  } else if (k_a->encode < k_b->encode) {
    return -1;
  }

  return ks_comp_kview_y_z(a, b);

}




int ks_comp_kview_e_yr(const void* a, const void* b) {
  KS_VIEW* k_a =(KS_VIEW*)a;
  KS_VIEW* k_b =(KS_VIEW*)b;

  if (k_a->encode > k_b->encode) {
    return 1;
  } else if (k_a->encode < k_b->encode) {
    return -1;
  }

  return ks_comp_kview_y_r(a, b);

}




int ks_comp_kview_e_zr(const void* a, const void* b) {
  KS_VIEW* k_a =(KS_VIEW*)a;
  KS_VIEW* k_b =(KS_VIEW*)b;

  if (k_a->encode > k_b->encode) {
    return 1;
  } else if (k_a->encode < k_b->encode) {
    return -1;
  }

  return ks_comp_kview_z_r(a, b);

}


int ks_comp_kview_e_descend_user1(const void* a, const void* b) {
  KS_VIEW* k_a =(KS_VIEW*)a;
  KS_VIEW* k_b =(KS_VIEW*)b;

  if (k_a->encode < k_b->encode) {
    return 1;
  } else if (k_a->encode > k_b->encode) {
    return -1;
  }

  return ks_comp_kview_user1(a, b);
}


int ks_comp_kview_e_user1(const void* a, const void* b) {
  KS_VIEW* k_a =(KS_VIEW*)a;
  KS_VIEW* k_b =(KS_VIEW*)b;

  if (k_a->encode > k_b->encode) {
    return 1;
  } else if (k_a->encode < k_b->encode) {
    return -1;
  }

  return ks_comp_kview_user1(a, b);
}



int ks_comp_kview_e_sec_t(const void* a, const void* b) {
    KS_VIEW* k_a =(KS_VIEW*)a;
    KS_VIEW* k_b =(KS_VIEW*)b;

    if (k_a->encode > k_b->encode) {
      return 1;
    } else if (k_a->encode < k_b->encode) {
      return -1;
    }

    const int N = 4;
    int segments = 128;
    float ta = fmod(N * (k_a->coord->t + PI), 2*PI) / (2 * PI);
    float tb = fmod(N * (k_b->coord->t + PI), 2*PI) / (2 * PI);
    int ca = (int)(ta * segments) - segments / 2;
    int cb = (int)(tb * segments) - segments / 2;

    if (ca < 0) {
      ca = -2 * ca - 1;
    } else {
      ca *= 2;
    }

    if (cb < 0) {
      cb = -2 * cb - 1;
    } else {
      cb *= 2;
    }

    if (ca > cb) {
      return 1;
    } else if (ca < cb) {
      return -1;
    }

    if (k_a->coord->t > k_b->coord->t) {
      return 1;
    } else if (k_a->coord->t < k_b->coord->t) {
      return -1;
    }

    return 0;

}




int ks_comp_kview_y_z_descend(const void* a, const void* b) {
    return -ks_comp_kview_y_z(a, b);
}




int ks_comp_kview_z_y_descend(const void* a, const void* b) {
    return -ks_comp_kview_z_y(a, b);
}


int ks_comp_kcoord_user1(const void* a, const void* b) {
    KS_KCOORD* k_a =(KS_KCOORD*)a;
    KS_KCOORD* k_b =(KS_KCOORD*)b;
    if (k_a->user1 > k_b->user1) {
      return 1;
    } else if (k_a->user1 < k_b->user1) {
      return -1;
    }
    return 0;
}

int ks_comp_kcoord_user1_descend(const void* a, const void* b) {
    return -ks_comp_kcoord_user1(a, b);
}

int ks_comp_kcoord_y_z_descend(const void* a, const void* b) {
    return -ks_comp_kcoord_y_z(a, b);
}

int ks_comp_kcoord_z_y_descend(const void* a, const void* b) {
    return -ks_comp_kcoord_z_y(a, b);
}

int ks_comp_kcoord_r_t_descend(const void* a, const void* b) {
    return -ks_comp_kcoord_r_t(a, b);
}

int ks_comp_kcoord_t_r_descend(const void* a, const void* b) {
    return -ks_comp_kcoord_t_r(a, b);
}

ks_comparator ks_get_comp_kview_y_z(int ascend) {
    return (ascend == 1) ? ks_comp_kview_y_z : ks_comp_kview_y_z_descend;
}


ks_comparator ks_get_comp_kview_z_y(int ascend) {
    return (ascend == 1) ? ks_comp_kview_z_y : ks_comp_kview_z_y_descend;
}


ks_comparator ks_get_comp_kcoord_user1(int ascend) {
    return (ascend == 1) ? ks_comp_kcoord_user1 : ks_comp_kcoord_user1_descend;
}

ks_comparator ks_get_comp_kview_e_user1(int reversed) {
  return (reversed == 1) ? ks_comp_kview_e_descend_user1 : ks_comp_kview_e_user1;
}

void ks_get_matrix_from_kcoords(KS_KCOORD* K, const int num_coords, int* y, int* z) {
  if (!K) {
    KS_THROW("K is NULL");
    return;
  }

  if (y == NULL && z == NULL) {
    KS_THROW("y and z cannot both be null");
    return;
  }

  if (y != NULL) {
    qsort(K, num_coords, sizeof(KS_KCOORD), &ks_comp_kcoord_dist_y_z);
    *y = K[num_coords - 1].y;
    if (*y > 0) {
      (*y)++;  /* Increment since coordinates are left-aligned */
    } else {
      *y = -(*y);
    }
    *y *= 2;
  }

  if (z != NULL) {
    qsort(K, num_coords, sizeof(KS_KCOORD), &ks_comp_kcoord_dist_z_y);
    *z = K[num_coords - 1].z;
    if (*z > 0) {
      (*z)++;
    } else {
      *z = -(*z);
    }
    *z *= 2;
  }

  ks_dbg("Matrix [Y, Z] = [%d, %d]", y == NULL ? 0 : *y, z == NULL ? 0 : *z);
  return;
}




void ks_pe_linear(int* view_order, int ETL) {
  int i;
  for (i = 0; i < ETL; i++) {
    view_order[i] = i;
  }
}




void ks_pe_linear_roll(int* view_order, int ETL, int c) {
  int base[ETL];
  int idx;
  for (idx = 0; idx < ETL; idx++) {
    base[idx] = idx;
  }
  int s;
  if (c < (ETL/2)) {
    s = ETL/2 - c;
  } else {
    s = 3*ETL/2 - c;
  }

  memcpy(&(view_order[0]), &(base[ETL-s]), sizeof(int) * s);
  memcpy(&(view_order[s]), &(base[0]), sizeof(int) * (ETL-s));
}




void ks_pe_pivot_roll(int* view_order, int ETL, int c) {
  int base[ETL];
  ks_pivot_specific_center_symk(base, ETL, 0);
  int s;
  if (c == 0) {
    s = 0;
  } else {
    if (ETL % 2 ) {
      if (c % 2) {
        s = ETL - c/2 - 1;
      } else {
        s = c / 2;
      }
    } else {
      if (c % 2) {
        s = ETL - c/2;
      } else {
        s = c / 2;
      }
    }
  }

  memcpy(&(view_order[0]), &(base[ETL-s]), sizeof(int) * s);
  memcpy(&(view_order[s]), &(base[0]), sizeof(int) * (ETL-s));
}




/* LCPO */
void ks_pivot_linear_center_symk(int* view_order, int ETL, int c) {

  typedef enum {
    UP,
    DOWN
  } UPDOWN;

  typedef enum {
    LEFT,
    RIGHT
  } LR;
  int odd_ETL = ETL % 2; 
  int idx = 0;

  if (c == ETL-1 && odd_ETL != 1) {
    return ks_pivot_linear_center_symk(view_order, ETL, c-1);
  }
  UPDOWN centertype =  c < (ETL/2) ? UP 
                                   : DOWN;

  /* Pivoting from center (replace later) */
  int pivot_index = (ETL-1)/2;
  if (odd_ETL != 1 && centertype == DOWN) {
    pivot_index++;
  }

  LR s_dir = RIGHT;
  if (centertype == DOWN && odd_ETL != 1) {
    s_dir = LEFT;
  } else if (centertype == UP && odd_ETL == 1) {
    s_dir = LEFT;
  }
  int val = centertype == DOWN ? ETL-1
                             : 0;
  for (idx = 0; idx < (ETL/2); idx++) {
    if (centertype == UP) {
      if (s_dir == LEFT) {
        view_order[pivot_index + idx] = val++;
        view_order[pivot_index - idx - 1] = val++;
      } else {
        view_order[pivot_index - idx] = val++;
        view_order[pivot_index + idx + 1] = val++;
      }
    } else {
      if (s_dir == LEFT) {
        view_order[pivot_index + idx] = val--;
        view_order[pivot_index - idx - 1] = val--;
      } else {
        view_order[pivot_index - idx] = val--;
        view_order[pivot_index + idx + 1] = val--;
      }
    }
  }

  if (odd_ETL == 1) {
    if (centertype == UP) {
      view_order[ETL-1] = ETL-1;
    } else {
      view_order[0] = 0;
    }

  }


  /* Linear sweep */
  const int linear_start = odd_ETL ? abs(ETL/2 - c)
                                   : abs(ETL/2 - 1 - c);
  const int num_linear = ETL - 2*linear_start;
  for (idx=0; idx < num_linear; idx++) {
    view_order[linear_start + idx] = centertype == UP ? idx
                                                     : linear_start*2 + idx;
  }
}




void ks_pivot_specific_center_symk(int* view_order, int ETL, int c) {
  /* Some enums for YOUR convenience */
  typedef enum {
    UP,
    DOWN
  } UPDOWN;

  typedef enum {
    LEFT,
    RIGHT
  } LR;

  /* Is center a peak or valley? */
  UPDOWN centertype =  c < (ETL/2) ? DOWN 
                                   : UP;
  int pivot = centertype == DOWN ? (int)(ceil((ETL - c)/ 2.0) - 1)
                                 : (int)(ceil((c   - 1)/ 2.0) + 1);
  if (centertype == UP) {
    pivot -= 1;
  }
  LR s_dir;

  if (ETL % 2) {
    s_dir = (c % 2) ? RIGHT : LEFT;
  } else {
    if (centertype == DOWN) {
      s_dir = (c % 2) ? LEFT : RIGHT;
    } else {
      s_dir = (c % 2) ? RIGHT : LEFT;
    }
  }
  int idx = 0;
  int val = centertype == DOWN ? 0 : (ETL-1);
  view_order[pivot] = (centertype == DOWN ? 0 : (ETL-1));
  for (; idx < pivot; idx++) {
    if (centertype == DOWN) {
      if (s_dir == LEFT) {
        view_order[pivot - idx - 1] = ++val;
        view_order[pivot + idx + 1] = ++val;
      } else {
        view_order[pivot + idx + 1] = ++val;
        view_order[pivot - idx - 1] = ++val;
      }
    } else {
      if (s_dir == LEFT) {
        view_order[pivot - idx - 1] = --val;
        view_order[pivot + idx + 1] = --val;
      } else {
        view_order[pivot + idx + 1] = --val;
        view_order[pivot - idx - 1] = --val;
      }
    }
  }
  for (idx = (pivot*2+1); idx < ETL; idx++) {
    view_order[idx] = (centertype == DOWN) ? idx
                                           : ((ETL-1-idx));
  }

  /* If center echo matches linear sweep */
  if (ETL/2 == c) {
    for (idx=0; idx < ETL; idx++) {
      view_order[idx] = idx;
    }
  }

}




void ks_pivot_specific_center_radk(int* view_order, int ETL, int c) {
    /* Some enums for YOUR convenience */
  typedef enum {
    UP,
    DOWN
  } UPDOWN;

  typedef enum {
    LEFT,
    RIGHT
  } LR;

  /* Is center a peak or valley? */
  UPDOWN centertype =  c < (ETL/2) ? DOWN 
                                   : UP;
  int pivot = centertype == DOWN ? (int)floor((  1 + c) / 2.0)
                                 : (int)floor((ETL - c) / 2.0);

  LR s_dir;
  if (ETL % 2) { /* Odd ETL */
    s_dir = (c % 2) ? RIGHT : LEFT;
  } else { /* Even ETL */
    if (centertype == DOWN) {
      s_dir = (c % 2) ? LEFT : RIGHT;
    } else {
      s_dir = (c % 2) ? RIGHT : LEFT;
    }
  }
  int idx = 0;
  int val = centertype == DOWN ? 0 : (ETL-1);
  view_order[pivot] = (centertype == DOWN ? 0 : (ETL-1));


  for (; idx < pivot; idx++) {
    if (centertype == DOWN) {
      if (s_dir == LEFT) {
        view_order[pivot - idx - 1] = ++val;
        view_order[pivot + idx + 1] = ++val;
      } else {
        view_order[pivot + idx + 1] = ++val;
        view_order[pivot - idx - 1] = ++val;
      }
    } else {
      if (s_dir == LEFT) {
        view_order[pivot - idx - 1] = --val;
        view_order[pivot + idx + 1] = --val;
      } else {
        view_order[pivot + idx + 1] = --val;
        view_order[pivot - idx - 1] = --val;
      }
    }
  }
  for (idx = (pivot*2+1); idx < ETL; idx++) {
    view_order[idx] = (centertype == DOWN) ? idx
                                           : ((ETL-1-idx));
  }

}




int ks_readout_start_index(int center, int etl, int num_encodes_per_train) {
  int readout_start = center - num_encodes_per_train/2;
  readout_start = IMax(2, readout_start, 0);
  readout_start = IMin(2, readout_start, etl - num_encodes_per_train);
  return readout_start;
}




KS_PEPLAN_SHOT_DISTRIBUTION ks_peplan_distribute_shots(const int etl, const int num_coords, const int center) {
  KS_PEPLAN_SHOT_DISTRIBUTION shot_distribution;

  shot_distribution.shots = CEIL_DIV(num_coords, etl);

  const int num_echoes = shot_distribution.shots * etl;
  const int num_extra_per_train = FLOOR_DIV(num_echoes - num_coords, shot_distribution.shots);

  shot_distribution.encodes_per_shot = etl - num_extra_per_train;

  shot_distribution.readout_start = center == KS_NOTSET ? KS_NOTSET
                                                        : ks_readout_start_index(center, etl, shot_distribution.encodes_per_shot);

  const int second_blocks = shot_distribution.encodes_per_shot * shot_distribution.shots - num_coords;
  shot_distribution.first_blocks = shot_distribution.shots - second_blocks;

  return shot_distribution;
}




int ks_peplan_find_center_from_linear_sweep(KS_KCOORD* const K, KS_VIEW* views_in,
                                            const int num_coords,
                                            const KS_MTF_DIRECTION mtf_direction,
                                            const int sweep_sign,
                                            const int segment_size,
                                            const int start_encode) {
  int idx;
  if (sweep_sign != 1 && sweep_sign != -1) {
    KS_THROW("Sweep sign must be +-1, not%d", sweep_sign);
    return KS_NOTSET;
  }
  KS_VIEW* views = (KS_VIEW*) malloc(num_coords * sizeof(KS_VIEW));
  if (views_in == NULL) {
    if (K == NULL) {
      KS_THROW("K and views cannot both be null");
      return KS_NOTSET;
    }

    for (idx = 0; idx < num_coords; idx++) {
      views[idx].coord = &(K[idx]);
    }
  } else if (K != NULL) {
      KS_THROW("Both K and views != NULL");
      return KS_NOTSET;
  } else {
    memcpy(views, views_in, num_coords * sizeof(KS_VIEW));
  }

  if (mtf_direction == MTF_Z) {
    qsort(views, num_coords, sizeof(KS_VIEW), ks_get_comp_kview_z_y(sweep_sign));
  } else if (mtf_direction == MTF_Y) {
    qsort(views, num_coords, sizeof(KS_VIEW), ks_get_comp_kview_y_z(sweep_sign));
  } else if (mtf_direction == MTF_R) {
    qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_r_t);
  }

  for (idx = 0; idx < num_coords; idx++) {
    views[idx].encode = idx / segment_size + start_encode;
  }

  if (mtf_direction == MTF_Y) {
    qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_dist_y_z);
    /* qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_r_t); */

  } else if (mtf_direction == MTF_Z) {
    qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_dist_z_y);
    /* qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_r_t); */

  } else if (mtf_direction == MTF_R) {
    qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_r_t);
  }
  int center = views[0].encode;
  /* ks_dbg("Center %d at [%d,%d]", views[0].encode, views[0].coord->y, views[0].coord->z); */


  free(views);


  return center;
}




STATUS ks_peplan_assign_shots(KS_PHASEENCODING_PLAN* peplan,
                              const KS_VIEW* const views,
                              const int num_views,
                              const KS_PEPLAN_SHOT_DISTRIBUTION* const shot_dist,
                              const int first_shot_number,
                              const int y_offset, const int z_offset,
                              const int is3D
                              /* Support negative readout stride? */) {
  const int readout_end = shot_dist->readout_start + shot_dist->encodes_per_shot;
  int readout = shot_dist->readout_start;
  int assigned = 0;
  for (; readout < readout_end; readout++) {
    int shot = first_shot_number;
    for (; shot < (shot_dist->shots + first_shot_number); shot++) {
      int block = (shot - first_shot_number) < shot_dist->first_blocks ? 0 : 1;
      if (block == 1 && readout == (readout_end -1)) {
        /* Second block - skip one encode */
        continue;
      } else {
        ks_phaseencoding_set(peplan,
                             views[assigned].encode,
                             shot,
                             views[assigned].coord->y + y_offset,
                             is3D ? views[assigned].coord->z + z_offset : KS_NOTSET);
        assigned++;
      }
    }
  }
  if (assigned != num_views) {
    return KS_THROW("Couldn't assign all requested encodes. There are %d encodes in shot_dist but %d were requested", assigned, num_views);
  }
  return SUCCESS;
}



STATUS ks_peplan_assign_encodes(KS_PHASEENCODING_PLAN* peplan,
                                const KS_VIEW* const views,
                                const int num_views,
                                const KS_PEPLAN_SHOT_DISTRIBUTION* const shot_dist,
                                const int first_shot_number,
                                const int y_offset, const int z_offset,
                                const int is3D
                                /* Support negative readout stride? */) {
  const int readout_end = shot_dist->readout_start + shot_dist->encodes_per_shot;
  int assigned = 0;
  int shot = first_shot_number;
  for (; shot < (shot_dist->shots + first_shot_number); shot++) {
    int block = (shot - first_shot_number) < shot_dist->first_blocks ? 0 : 1;
    int readout = shot_dist->readout_start;
    for (; readout < readout_end; readout++) {
      if (block == 1 && readout == (readout_end - 1)) {
        /* Second block - skip one encode */
        continue;
      } else {
        ks_phaseencoding_set(peplan,
                             readout,
                             views[assigned].shot,
                             views[assigned].coord->y + y_offset,
                             is3D ? views[assigned].coord->z + z_offset : KS_NOTSET);
        assigned++;
      }
    }
  }
  if (assigned != num_views) {
    return KS_THROW("Couldn't assign all requested encodes. There are %d encodes in shot_dist but %d were requested", assigned, num_views);
  }
  return SUCCESS;
}



static void ks_assign_views_to_contiguous_shots(KS_VIEW* views,
                                                KS_KCOORD* coords,
                                                const int num_coords,
                                                const KS_PEPLAN_SHOT_DISTRIBUTION* const shot_dist) {
  int shot = 0;
  int views_in_shot = 0;
  const int views_per_short_shot = shot_dist->encodes_per_shot - 1;
  int views_in_current_shot = shot < shot_dist->first_blocks ? shot_dist->encodes_per_shot : views_per_short_shot;
  int idx;

  for (idx = 0; idx < num_coords; idx++) {
    if (views_in_shot == views_in_current_shot) {
      shot++;
      views_in_shot = 0;
      views_in_current_shot = shot < shot_dist->first_blocks ? shot_dist->encodes_per_shot : views_per_short_shot;
    }
    views[idx].coord = coords + idx;
    views[idx].encode = KS_NOTSET;
    views[idx].shot = shot;
    views_in_shot++;
  }
}



void _pe_set__theta(KS_KCOORD* K, const int num_coords, const int y0, const int z0) {
  int idx;
  for (idx = 0; idx < num_coords; idx++) {
    const float y_dist = K[idx].y - y0;
    const float z_dist = K[idx].z - z0;
    K[idx].t = atan2(y_dist, z_dist);
  }
}

void set_chevron_radius(KS_KCOORD* K, const int num_coords, const int y0, const int z0, const float axis_ratio) {
  int idx;
  for (idx = 0; idx < num_coords; idx++) {
    const float y_dist = abs(K[idx].y - y0);
    const float z_dist = abs(K[idx].z - z0);
    K[idx].user1 = y_dist + z_dist * axis_ratio;
  }
}


void set_eclipse_radius(KS_KCOORD* K, const int num_coords, const int y0, const int z0, const float axis_ratio) {
  int idx;
  for (idx = 0; idx < num_coords; idx++) {
    const float y_dist = abs(K[idx].y - y0);
    const float z_dist = abs(K[idx].z - z0);
    K[idx].user1 = sqrt(y_dist * y_dist + 
                        z_dist * z_dist * axis_ratio * axis_ratio);
  }
}
typedef void (*radius_function_t)(KS_KCOORD*, const int, const int, const int, const float);

STATUS ks_generate_peplan_from_kcoords_american_3d(const KS_KSPACE_ACQ* kacq,
                                                   KS_PHASEENCODING_PLAN* peplan,
                                                   const KS_VIEW_MTF_PROFILE mtf_profile,
                                                   const int etl,
                                                   const int center /* Center echo in k-space [0, etl-1]*/) {
  
  if (center < 0 || center >= etl) {
    return KS_THROW("Center echo must be in [0, etl-1]");
  }

  if (kacq->matrix_size[ZGRAD] <= 1) {
    return KS_THROW("Must be 3D coordinates");
  }

  int idx;
  STATUS s;

  /* Only difference between eclipse and chevron is how the radii are calculated */
  radius_function_t set_radius = NULL;
  if (mtf_profile == CHEVRON) {
    set_radius = set_chevron_radius;
  } else if (mtf_profile == ECLIPSE) {
    set_radius = set_eclipse_radius;
  } else {
    return KS_THROW("Unsupported MTF profile");
  }

  /* Allocate views */
  KS_VIEW* views = (KS_VIEW*) malloc(kacq->num_coords * sizeof(KS_VIEW));

  /* Distribute shots to get readout_start and encodes_per_shot */
  KS_PEPLAN_SHOT_DISTRIBUTION shot_dist = ks_peplan_distribute_shots(etl, kacq->num_coords, center);

  /* Ellipse origin and axis_ratio */
  const int y0 = kacq->matrix_size[YGRAD] / 2;
  const int z0 = 0;
  float axis_ratio = 1.0;
  
  /* Set theta according to the specified ellipse origin */
  _pe_set__theta(kacq->coords, kacq->num_coords, y0, z0);

  /* Binary search parameters */
  const int max_iter = 20;
  int iter = 0;
  float left_axis_ratio = 0.005;
  float right_axis_ratio = 500;
  int cur_center = INT_MAX;
  
  /* Binary search to get axis ratio for desired center encode */
    KS_THROW("Desired center = %d", center);
    while ( (cur_center != center) && (iter < max_iter) ) {
    axis_ratio = (left_axis_ratio + right_axis_ratio) / 2;

    /* Step logarithmically instead */
    /* axis_ratio = sqrt(left_axis_ratio * right_axis_ratio); */

    set_radius(kacq->coords, kacq->num_coords, y0, z0, axis_ratio);
    
    /* Sort coordinates by user1 (ellipse radius) in ascending order */
    qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);

    /* Assign encodes */
    for (idx = 0; idx < kacq->num_coords; idx++) {
      views[idx].coord = kacq->coords + idx;
      views[idx].encode = idx / shot_dist.shots;
    }

    /* Sort by origin distance to get center encode */
    qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
    cur_center = views[0].encode;
    
    if (cur_center > center) {
      left_axis_ratio = axis_ratio;
    } else {
      right_axis_ratio = axis_ratio;
    }

    KS_THROW("Iter %d: Center encode %d, axis ratio %.2f, left_axis_ratio %.2f, right_axis_ratio %.2f", iter, cur_center, axis_ratio, left_axis_ratio, right_axis_ratio);
    iter++;
  }

  /* Track whether the native search succeeded before fine-tuning may change cur_center */
  int found_native = (cur_center == center);
  int reversed = 0;

  /* Fine-tuning step: increase axis_ratio until cur_center is not equal to center */
  float axis_ratio_fine_tune = axis_ratio;
  while ( (cur_center == center) && (axis_ratio_fine_tune < 10) ) {
    axis_ratio_fine_tune *= 1.01f;

    set_radius(kacq->coords, kacq->num_coords, y0, z0, axis_ratio_fine_tune);

    /* Sort coordinates by user1 (ellipse radius) in ascending order */
    qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);

    /* Assign encodes */
    for (idx = 0; idx < kacq->num_coords; idx++) {
      views[idx].coord = kacq->coords + idx;
      views[idx].encode = idx / shot_dist.shots;
    }

    /* Sort by origin distance to get center encode */
    qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
    cur_center = views[0].encode;
    if (cur_center == center) {
      axis_ratio = axis_ratio_fine_tune;
    }

    KS_THROW("Fine-tuning: Center encode %d, axis ratio %.2f", cur_center, axis_ratio_fine_tune);
  }

  if (!found_native) {
    /* Native search failed — try the reversed center (etl-1-center) and
     * flip all encode assignments afterwards so the k-space origin still maps to center. */
    const int rev_center = etl - 1 - center;
    KS_THROW("Primary search failed (got %d, want %d). Trying reversed center %d", cur_center, center, rev_center);

    iter = 0;
    left_axis_ratio = 0.005;
    right_axis_ratio = 500;
    cur_center = INT_MAX;

    while ( (cur_center != rev_center) && (iter < max_iter) ) {
      axis_ratio = (left_axis_ratio + right_axis_ratio) / 2;

      set_radius(kacq->coords, kacq->num_coords, y0, z0, axis_ratio);

      /* Sort coordinates by user1 (ellipse radius) in ascending order */
      qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);

      /* Assign encodes */
      for (idx = 0; idx < kacq->num_coords; idx++) {
        views[idx].coord = kacq->coords + idx;
        views[idx].encode = idx / shot_dist.shots;
      }

      /* Sort by origin distance to get center encode */
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
      cur_center = views[0].encode;

      if (cur_center > rev_center) {
        left_axis_ratio = axis_ratio;
      } else {
        right_axis_ratio = axis_ratio;
      }

      KS_THROW("Rev iter %d: Center encode %d, axis ratio %.2f", iter, cur_center, axis_ratio);
      iter++;
    }

    /* Track whether the reversed binary search succeeded before fine-tuning may change cur_center */
    int found_reversed = (cur_center == rev_center);

    /* Fine-tuning for reversed center */
    float axis_ratio_fine_tune_rev = axis_ratio;
    while ( (cur_center == rev_center) && (axis_ratio_fine_tune_rev < 10) ) {
      axis_ratio_fine_tune_rev *= 1.01f;

      set_radius(kacq->coords, kacq->num_coords, y0, z0, axis_ratio_fine_tune_rev);

      qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);

      for (idx = 0; idx < kacq->num_coords; idx++) {
        views[idx].coord = kacq->coords + idx;
        views[idx].encode = idx / shot_dist.shots;
      }

      qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
      cur_center = views[0].encode;
      if (cur_center == rev_center) {
        axis_ratio = axis_ratio_fine_tune_rev;
      }

      KS_THROW("Rev fine-tuning: Center encode %d, axis ratio %.2f", cur_center, axis_ratio_fine_tune_rev);
    }

    if (found_reversed) {
      KS_THROW("----- Found reversed solution: rev_center %d at axis ratio %.2f ---", rev_center, axis_ratio);
      reversed = 1;
    } else {
      KS_THROW("----- Could not find native or reversed solution (wanted %d or %d) -----", center, rev_center);
    }
  }

  /* Now we know the final axis_ratio, set ellipse radius and assign one final time */
  set_radius(kacq->coords, kacq->num_coords, y0, z0, axis_ratio);

  /* Sort coordinates by user1 (ellipse radius) in ascending order */
  qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);

  /* Assign encodes */
  for (idx = 0; idx < kacq->num_coords; idx++) {
    views[idx].coord = kacq->coords + idx;
    views[idx].encode = idx / shot_dist.shots;
  }

  /* Sort by origin distance to get center encode */
  qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
  KS_THROW("Final axis_ratio %.2f, center = %d", axis_ratio, views[0].encode);

  /* If using reversed solution, flip all encode assignments so k-space origin maps to echo [center] */
  if (reversed) {
    for (idx = 0; idx < kacq->num_coords; idx++) {
      views[idx].encode = etl - 1 - views[idx].encode;
    }
    KS_THROW("After flipping, center encode = %d", views[0].encode);
  }

  /* Sort by theta (from ellipse origin) to get the "radar" shot order */
  qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_e_tr);

  /* Allocate peplan */
  s = ks_phaseencoding_alloc(peplan, etl, shot_dist.shots);
  if (s != SUCCESS) {
    free(views);
    return s;
  }

  /* Assign shots */
  s = ks_peplan_assign_shots(peplan, views, kacq->num_coords, &shot_dist, 0, kacq->matrix_size[YGRAD]/2, kacq->matrix_size[ZGRAD]/2, 1);

  free(views);
  return SUCCESS;
}

int _croc_search(
  const KS_KSPACE_ACQ* kacq,
  KS_VIEW* views,
  const int shots,
  YZ start,
  YZ end,
  int center,
  int largest_sector,
  YZ* out_center,
  float* yz_ratio_out
) {
  // Do a binary search along the sector midline to find a matching center. Start midway
  int cur_center = KS_NOTSET;
  int max_iter = 10;
  int iter = 0;
  int idx;

  YZ new_center = {-1, -1};

  //KS_THROW("-------------------------------------------");
  //KS_THROW("Binary search for center encode %d", center);
  //KS_THROW("-------------------------------------------");
  const float ymax = (float)kacq->matrix_size[YGRAD] / 2;
  const float zmax = (float)kacq->matrix_size[ZGRAD] / 2;

  /* Pre-check: test the max offset position (end). If even that cannot reach the desired
     center encode, the binary search has no solution and we skip straight to elliptical. */
  {
    for (idx = 0; idx < kacq->num_coords; idx++) {
      const float y_dist = abs(kacq->coords[idx].y - end.y) / ymax;
      const float z_dist = abs(kacq->coords[idx].z - end.z) / zmax;
      kacq->coords[idx].user1 = sqrt(y_dist * y_dist + z_dist * z_dist);
    }
    qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);
    for (idx = 0; idx < kacq->num_coords; idx++) {
      views[idx].coord = kacq->coords + idx;
      views[idx].encode = idx / shots;
    }
    qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
    int max_achievable = views[0].encode;
    if (max_achievable < center) {
      /* Best position is still too low: skip binary search, start elliptical from end */
      new_center = end;
      cur_center = max_achievable;
      iter = max_iter+1;
    }
  }

  while (center != cur_center && iter < max_iter) {
    YZ prev_center = new_center;
    new_center.y = (end.y + start.y) / 2;
    new_center.z = (end.z + start.z) / 2;
    /* Integer binary search has converged: adjacent start/end produce the same midpoint */
    if (new_center.y == prev_center.y && new_center.z == prev_center.z) {
      break;
    }
    /* KS_THROW("Iter %d: Center encode %d, start = [%d,%d], end = [%d,%d], new_center = [%d,%d]", iter, cur_center, start.y, start.z, end.y, end.z, new_center.y, new_center.z); */

    // Assign radius from the new center to user1
    for (idx = 0; idx < kacq->num_coords; idx++) {
      const float y_dist = abs(kacq->coords[idx].y - new_center.y) / ymax;
      const float z_dist = abs(kacq->coords[idx].z - new_center.z) / zmax;
      kacq->coords[idx].user1 = sqrt(y_dist * y_dist + z_dist * z_dist);
    }

    /* Sort coordinates by the new center */
    qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);

    /* Assign encodes */
    for (idx = 0; idx < kacq->num_coords; idx++) {
      views[idx].coord = kacq->coords + idx;
      views[idx].encode = idx / shots;
    }

    /* Sort by origin distance to get center encode */
    qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
    cur_center = views[0].encode;

    if (cur_center > center) {
      // Move closer to origin
      end.y = new_center.y;
      end.z = new_center.z;
      //KS_THROW("cur_center (%d) too large, moving closer to origin", cur_center);
    } else if (cur_center < center) {
      // Move away from origin
      start.y = new_center.y;
      start.z = new_center.z;
      //KS_THROW("cur_center (%d) too small, moving away from origin", cur_center);
    } else {
      // We found it!
      break;
    }

    iter++;
  } /* Binary search */
  
    KS_THROW("Used %d iter", iter);


  /* Search elliptically if solution not found.
     Start stepping in the direction that reduces the error. Flip on overshoot. */
  float yz_ratio = 1.0f;
  float yz_ratio_step = (largest_sector == WEST || largest_sector == EAST) ? 0.8f : 1.2f;

  while (center != cur_center && yz_ratio > 0.05f && yz_ratio < 20.0f) {
    int prev_cur_center = cur_center;
    yz_ratio *= yz_ratio_step;

    // Assign radius from the new center to user1
    for (idx = 0; idx < kacq->num_coords; idx++) {
      const float y_dist = abs(kacq->coords[idx].y - new_center.y) / ymax * yz_ratio;
      const float z_dist = abs(kacq->coords[idx].z - new_center.z) / zmax;
      kacq->coords[idx].user1 = sqrt(y_dist * y_dist + z_dist * z_dist);
    }

    /* Sort coordinates by the new center */
    qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);

    /* Assign encodes */
    for (idx = 0; idx < kacq->num_coords; idx++) {
      views[idx].coord = kacq->coords + idx;
      views[idx].encode = idx / shots;
    }

    /* Sort by origin distance to get center encode */
    qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
    cur_center = views[0].encode;

    /* Overshoot detection: if we crossed the target, reverse direction with halved magnitude */
    if (cur_center != center && (cur_center < center) != (prev_cur_center < center)) {
      yz_ratio_step = 1.0f + (1.0f - yz_ratio_step) * 0.5f;
    }
  }

  if (yz_ratio_out) {
    *yz_ratio_out = yz_ratio;
  }
  if (out_center) {
    *out_center = new_center;
  }
  
  return cur_center;
}


STATUS ks_generate_peplan_from_kcoords_CROC(const KS_KSPACE_ACQ* kacq,
                                            KS_PHASEENCODING_PLAN* peplan,
                                            const int etl /* echoes per shot */,
                                            const int center /* Center echo in k-space [0, etl-1]*/) {

  
  /* Distribute shots to get readout_start and encodes_per_shot */
  KS_PEPLAN_SHOT_DISTRIBUTION shot_dist = ks_peplan_distribute_shots(etl, kacq->num_coords, center);
  //KS_THROW("Shot distribution: %d shots, %d encodes per shot, readout start %d", shot_dist.shots, shot_dist.encodes_per_shot, shot_dist.readout_start);
  if (center < 0 || center >= etl) {
    return KS_THROW("Center echo must be in [0, etl-1] = [0, %d]", etl-1);
  }

  if (kacq->matrix_size[ZGRAD] <= 1) {
    return KS_THROW("Must be 3D coordinates");
  }
  int idx;
  STATUS s;

  /* Recalculate and sort by theta */
  for (idx = 0; idx < kacq->num_coords; idx++) {
    kacq->coords[idx].user1 = (float)atan2(kacq->coords[idx].y + .5f, kacq->coords[idx].z + .5f);
  } 
  
  qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_user1);
  ks_dbg("Sorted by elliptically adjusted theta");

  /* Sectors are pi/2 wide. Find the one with the most coordinates  */
  float cutoffs[4] = {-3*M_PI_4, 
                        -M_PI_4,
                         M_PI_4,
                       3*M_PI_4};
  int num_coords_per_sector[4] = {0, 0, 0, 0};
  int cur_sector = 0;
  float max_angle = cutoffs[0];

  for (idx = 0; idx < kacq->num_coords; idx++) {
    if (cur_sector <= 3 && (kacq->coords[idx].user1 > max_angle)) {
      /* We are in the next sector */
      max_angle = cutoffs[++cur_sector];
    }
    num_coords_per_sector[cur_sector % 4]++; /* First sector centers on theta=0, hence the modulo */
  }

  ks_dbg("Num coords per sector: %d %d %d %d", num_coords_per_sector[0], num_coords_per_sector[1], num_coords_per_sector[2], num_coords_per_sector[3]);
  
  /* Find the largest sector */
  int largest_sector = 0;
  for (idx = 0; idx < 4; idx++) {
    if (num_coords_per_sector[idx] >= num_coords_per_sector[largest_sector]) {
      largest_sector = idx;
    }
  }

  float croc_theta = cutoffs[largest_sector] - M_PI_4;
  ks_dbg("Max sector %d with %d coords. CROC theta = %.2f deg", largest_sector, num_coords_per_sector[largest_sector], croc_theta*180/M_PI);
  
  YZ start = {0, 0};
  YZ end = {0, 0};
  switch (largest_sector) {
    case NORTH:
      end.y = (kacq->matrix_size[YGRAD]) / 2;
      break;
    case EAST:
      end.z = (kacq->matrix_size[ZGRAD] - 1) / 2;
      break;
    case SOUTH: 
      end.y = -(kacq->matrix_size[YGRAD] - 1) / 2;
      break;
    case WEST:
      end.z = -(kacq->matrix_size[ZGRAD] ) / 2;
      break;
    default:
      return KS_THROW("Invalid sector %d", largest_sector);
  }

  ks_dbg("End = [%d,%d], start = [%d,%d]", end.y, end.z, start.y, start.z);

  /* Allocate views */
  KS_VIEW* views = (KS_VIEW*) malloc(kacq->num_coords * sizeof(KS_VIEW));
  
  /* Do the search */
  YZ new_center;
  float yz_ratio;
  int cur_center = _croc_search(kacq, views, shot_dist.shots, start, end, center, largest_sector, &new_center, &yz_ratio);

  int reversed = 0;
  if (center == cur_center) {
    ks_dbg("----- Final center encode %d at [%d,%d] with ratio %.2f ---", cur_center, new_center.y, new_center.z, yz_ratio);
  } else {
    ks_dbg("----- Trying reversed as final center encode %d != desired %d", cur_center, center);
    /* Try to find etl - 1 - center */
    int rev_center = etl - 1 - center;
    cur_center = _croc_search(kacq, views, shot_dist.shots, start, end, rev_center, largest_sector, &new_center, &yz_ratio);
    if (cur_center == rev_center) {
      ks_dbg("----- Found solution for reverse center encode %d at [%d,%d] with ratio %.2f ---", cur_center, new_center.y, new_center.z, yz_ratio);
      
      // Reverse the encodes
      for (idx = 0; idx < kacq->num_coords; idx++) {
        views[idx].encode = etl - 1 - views[idx].encode;
      }

      /* Double check the new center encode */
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_comp_kview_r_t);
      int cur_center = views[0].encode;
      ks_dbg("After flipping, cur_center is %d", cur_center);

      reversed = 1;
    } else {
      free(views);
      KS_THROW("----- COULD NOT FIND A REVERSED SOLUTION ----- returning 0 (FAILURE)");
      return FAILURE;
    }
  }
  
  float atan_x = 0.0f;
  float atan_y = 0.0f;
  KS_VIEW* view = views;
  for (idx = 0; idx < kacq->num_coords; idx++, view++) {
    float y = (view->coord->y - new_center.y + .5);
    float z = (view->coord->z - new_center.z + .5);
    if (largest_sector == WEST) {
      atan_x = -z;
      atan_y = -y;
    } else if (largest_sector == SOUTH) {
      atan_x = -y;
      atan_y = z;
    } else if (largest_sector == EAST) {
      atan_x = z;
      atan_y = y;
    } else if (largest_sector == NORTH) {
      atan_x = y;
      atan_y = -z;
    }
    float angle = atan2(atan_y, atan_x);
    if (angle < 0.0) {
      angle += 2 * M_PI; /* [0, 2pi] */
    }

    view->coord->user1 = angle; /* fmod(angle + M_PI, 2 * M_PI) - M_PI; */
  }

  /* Sort by theta (from ellipse origin) to get the "radar" shot order */
  qsort(views, kacq->num_coords, sizeof(KS_VIEW), ks_get_comp_kview_e_user1(reversed));
  

  /* Special case for encode 0 to avoid singularity */
  for (view = views, idx = 0; idx < shot_dist.shots; idx++, view++) {
    float z = view->coord->z - new_center.z + .5;
    float y = view->coord->y - new_center.y + .5;
    if (largest_sector == WEST) {
      view->coord->user1 = y;
    } else if (largest_sector == EAST) {
      view->coord->user1 = -y;
    } else if (largest_sector == SOUTH) {
      view->coord->user1 = -z;
    } else if (largest_sector == NORTH) {
      view->coord->user1 = z;
    }
  }
  /* Note: only encode 0  */
  qsort(views, shot_dist.shots, sizeof(KS_VIEW), ks_comp_kview_e_user1);

  /* Allocate peplan */
  s = ks_phaseencoding_alloc(peplan, etl, shot_dist.shots);
  if (s != SUCCESS) {
    free(views);
    return s;
  }

  /* Assign shots */
  s = ks_peplan_assign_shots(peplan, views, kacq->num_coords, &shot_dist, 0, kacq->matrix_size[YGRAD]/2, kacq->matrix_size[ZGRAD]/2, 1);
  
  free(views);
  return s;
}




STATUS ks_generate_peplan_from_kcoords_american_2d(const KS_KSPACE_ACQ* kacq,
                                                KS_PHASEENCODING_PLAN* peplan,
                                                const KS_MTF_DIRECTION mtf_direction,
                                                const int etl /* echoes per shot */,
                                                const int center /* Center echo in k-space [0, etl-1]*/) {

  KS_PEPLAN_SHOT_DISTRIBUTION unsplit_shots = ks_peplan_distribute_shots(etl, kacq->num_coords, center);

  int midshots, idx;
  KS_VIEW* views =(KS_VIEW*) malloc(kacq->num_coords * sizeof(KS_VIEW));

  int midcoords = 0;
  int readout, shot;
  int is3d = 0;

  KS_PEPLAN_SHOT_DISTRIBUTION mid_shots;
  KS_PEPLAN_SHOT_DISTRIBUTION outer_shots;
  /* Figure out center by sorting by radius */
  KS_KCOORD* K = kacq->coords;
  int center_line = mtf_direction == MTF_Z ? kacq->matrix_size[ZGRAD]
                                           : kacq->matrix_size[YGRAD];

  /* Create views and figure out if this is a 3D peplan */
  for (idx = 0; idx < kacq->num_coords; idx++) {
    is3d |= (K[0].z != K[idx].z);
    views[idx].coord = &(K[idx]);
    views[idx].shot = KS_NOTSET;
  }

  if (is3d == 0 && mtf_direction == MTF_Z) {
    free(views);
    return KS_THROW("Requested MTF along Z ordering with 2D coordinates");
  }

  int found_solution = 0;
  for (midshots = 1; midshots < unsplit_shots.shots; ) {
    midcoords = midshots * unsplit_shots.encodes_per_shot;
    mid_shots = ks_peplan_distribute_shots(etl, midcoords, center);
    int mid_readout_end = mid_shots.readout_start + mid_shots.encodes_per_shot;

    /* Reset encodes */
    for (idx = 0; idx < kacq->num_coords; idx++) {
      views[idx].encode = -1000;
    }

    /* Sort views z->y or y->z */
    if (mtf_direction == MTF_Z) {
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_z_y);
    } else {
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_y_z);
    }

    /* Assign encodes to midcoord views (center -> out) */
    idx = 0;
    for (readout = mid_readout_end-1; readout >= mid_shots.readout_start; readout--) {
      for (shot = 0; shot < midshots; shot++) {
        int block = shot < mid_shots.first_blocks ? 0 : 1;
        if (block == 1 && readout == (mid_readout_end -1)) {
          continue;
        }

        /* Only assign midcoords */
        if (idx < midcoords) {
          views[idx].encode = readout;
        }
        idx++;
      }
    }

    if (idx != midcoords) {
      KS_THROW("Something is wrong with the midcoords");
    }

    /* Sort midcoords encode->z or encode->y */
    if (mtf_direction == MTF_Z) {
      qsort(views, midcoords, sizeof(KS_VIEW), &ks_comp_kview_e_zy);
    } else {
      qsort(views, midcoords, sizeof(KS_VIEW), &ks_comp_kview_e_yz);
    }

    /* Figure out the center encode */
    int current_center_encode = KS_NOTSET;
    int center_distance_late = KS_NOTSET;
    int center_distance_early = KS_NOTSET;
    int e = KS_NOTSET;
    for (e = 0; e < mid_shots.encodes_per_shot; e++) {
      if (mtf_direction == MTF_Z) {
        center_distance_late = views[(e+1) * midshots - 1].coord->z - center_line;
        center_distance_early = views[e * midshots].coord->z - center_line;
      } else {
        center_distance_late = views[(e+1) * midshots - 1].coord->y - center_line;
        center_distance_early = views[e * midshots].coord->y - center_line;
      }
      if (center_distance_early <= 0 && center_distance_late >= 0) { /* distance 0 is inside the encode section */
        current_center_encode = e;
        found_solution = 1;
        break;
      }
    }
    if (current_center_encode == center) { /* Found it */
      break;
    } else {
      /* Incorrect center encode -> increase midshots and try again */
      midshots++;
      continue;
    }
  } /* for midshots */

  if (!found_solution) {
    free(views);
    return KS_THROW("Could not split shots to get center = %d", center);
  }
  /* Split encodes between remaining shots */
  const int rest_num_coords = kacq->num_coords - midcoords;
  outer_shots = ks_peplan_distribute_shots(etl, rest_num_coords, center);

  STATUS status = ks_phaseencoding_alloc(peplan, etl, outer_shots.shots + mid_shots.shots);
  KS_RAISE(status);

  const int outer_readout_end = outer_shots.readout_start + outer_shots.encodes_per_shot;
  /* Loop through the block (which is shifted to the desired encode) and assign the entries */
  for (readout = outer_shots.readout_start; readout < outer_readout_end; readout++) {
    for (shot = 0; shot < outer_shots.shots; shot++) {
      int block = shot < outer_shots.first_blocks ? 0 : 1;
      if (block == 1 && readout == (outer_readout_end -1)) {
        continue;
      }
      if (idx >= kacq->num_coords) {
        free(views);
        return KS_THROW("Trying to assign more coordinates than what is available");
      }
      views[idx++].encode = readout;
    }
  }

  /* Sort outer coordinates (either encode->z or encode->y) */
  if (mtf_direction == MTF_Z) {
    qsort(views + midcoords, rest_num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_zy);
  } else {
    qsort(views + midcoords, rest_num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_yz);
  }

  KS_RAISE(ks_peplan_assign_shots(peplan, views,                   midcoords,   &mid_shots,         0, kacq->matrix_size[YGRAD]/2, kacq->matrix_size[ZGRAD]/2, is3d)); 
  KS_RAISE(ks_peplan_assign_shots(peplan, views + midcoords, rest_num_coords, &outer_shots,  midshots, kacq->matrix_size[YGRAD]/2, kacq->matrix_size[ZGRAD]/2, is3d));

  free(views);
  return SUCCESS;
}




STATUS ks_generate_peplan_from_kcoords_dutch(const KS_KSPACE_ACQ* kacq,
                                             KS_PHASEENCODING_PLAN* peplan,
                                             KS_MTF_DIRECTION mtf_direction,
                                             const int etl /* echoes per shot */,
                                             const int center /* Center echo in k-space [0, etl-1]*/) {
  return ks_generate_peplan_from_kcoords_american_2d(kacq, peplan, mtf_direction, etl, center);
}




STATUS ks_assign_encodes(KS_VIEW* const views,
                         const int num_views,
                         const int start_encode,
                         const int num_encodes,
                         const int sign) {
  int idx;
  if (num_views < 0) {
    /*ks_dbg("Not assigning %d encodes at start %d", num_encodes, start_encode);*/
    return SUCCESS;
  }
  if (num_encodes < 1) {
    return KS_THROW("num_encodes (%d) must be > 0 ", num_encodes);
  }
  const int num_shots = CEIL_DIV(num_views, num_encodes);
  /*ks_dbg("Assigning %d views with %d encodes into %d shots at start %d with sign %d", num_views, num_encodes, num_shots, start_encode, sign);*/
  for (idx = 0; idx < num_views; idx++) {
    if (views[idx].encode != KS_NOTSET) {
      /* KS_THROW("view has an encode already (%d, wanted %d) [%d, %d]", views[idx].encode, start_encode + sign * idx / num_shots, views[idx].coord->y, views[idx].coord->z); */
    }
    views[idx].encode = start_encode + sign * idx / num_shots;
  }

  return SUCCESS;
}




STATUS ks_pe_permute_pclo_rad(int* view_order, const int etl, const int center) {
  if (view_order == NULL) {
    return KS_THROW("View order is NULL");
  }
  if (center < 0) {
    return KS_THROW("Center encode (%d) must be in [0, %d)", center, etl);
  }
  if (center > etl-1) {
    return KS_THROW("Center encode (%d) must be smaller than encodes per shot (%d)", center, etl);
  }
  int i;
  if (center >= (etl + 1)/2) {

    KS_RAISE(ks_pe_permute_pclo_rad(view_order, etl, etl-center-1));
    for (i=0; i < etl; i++) {
      view_order[i] = etl - 1 - view_order[i];
    }
    return SUCCESS;
  }
  const int num_inner = CEIL_DIV(center,2);
  const int num_outer = CEIL_DIV(center+1,2);
  const int num_linear = etl - num_inner - num_outer - 1;
  const int even_center_encode = center % 2 == 0;

  for (i = 0; i < num_inner; i++) {
    view_order[i] = center - 2*i;
  }

  view_order[i] = 0;


  for (i = 1; i <= num_outer; i++) {
    view_order[num_inner + i] = 2*i - even_center_encode;
  }

  for (i = 1; i <= num_linear; i++) {
    view_order[num_inner + num_outer + i] = num_inner + num_outer + i;
  }


  return SUCCESS;
}




STATUS ks_lcpo_assign_encodes(KS_VIEW* views,
                              const int num_coords,
                              KS_MTF_DIRECTION mtf_direction,
                              const int split_shots,
                              const int etl,
                              const int center)  {
  KS_PEPLAN_SHOT_DISTRIBUTION shot_dist = ks_peplan_distribute_shots(etl, num_coords, center);
  if (center > shot_dist.encodes_per_shot/2) {
    ks_lcpo_assign_encodes(views, num_coords, mtf_direction, split_shots, etl, etl - 1 - center);
    int idx = 0;
    for (; idx < num_coords; idx++) {
      views[idx].encode = etl - 1 - views[idx].encode;
    }
    return SUCCESS;
  }
  int num_inner_coords;
  int num_inner_encodes;
  /* Find the linear size that gets the desired center */
  /* Sort by distance from center along mtf_direction to ensure we get the central coordinates */
  if (mtf_direction == MTF_Y) {
    qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_dist_y_z);
  } else {
    qsort(views, num_coords, sizeof(KS_VIEW), &ks_comp_kview_dist_z_y);
  }
  int start_from = 0;

  /* Figure out sweep order (lth or htl) */
  num_inner_coords = IMin(2, shot_dist.encodes_per_shot * shot_dist.shots, num_coords);
  const int center_lth_linsweep = ks_peplan_find_center_from_linear_sweep(NULL, views, num_inner_coords, mtf_direction, +1 /* lth */, shot_dist.shots, start_from);
  const int center_htl_linsweep = ks_peplan_find_center_from_linear_sweep(NULL, views, num_inner_coords, mtf_direction, -1 /* htl */, shot_dist.shots, start_from);
  int sweep_sign = 0;
  /* ks_dbg("lth/htl %d/%d", center_lth_linsweep, center_htl_linsweep); */
  if (center <= center_lth_linsweep && center <= center_htl_linsweep) {
    /* both possible */
    sweep_sign = center_lth_linsweep < center_htl_linsweep ? 1 : -1; /* Prefer starting from short side */
    /* ks_dbg("Two solutions - prefer %d", sweep_sign); */
  } else if (center <= center_lth_linsweep) {
    /* ks_dbg("Must do low to high"); */
    sweep_sign = 1; /* low to high */
  } else if (center <= center_htl_linsweep) {
    /* ks_dbg("Must do high to low"); */
    sweep_sign = -1; /* high to low */
  } else {
    /* ks_dbg("lth / htl = %d / %d", center_lth_linsweep, center_htl_linsweep); */
    if ((center > 0) && (etl % 2 == 0)) { /* Even ETL is a bit tricky with the center definition */
     /* is htl == lth a better condition? */
      KS_THROW("Center encode %d cannot be achieved - trying %d", center, center-1);
      return ks_lcpo_assign_encodes(views, num_coords, mtf_direction, split_shots, etl, center -1);
    } else {
      return KS_THROW("Center encode %d cannot be achieved", center);
    }

  }

  for (num_inner_encodes = shot_dist.encodes_per_shot; num_inner_encodes > 0; num_inner_encodes--) {
    num_inner_coords = IMin(2, num_inner_encodes * shot_dist.shots, num_coords);
    const int current_center_encode = ks_peplan_find_center_from_linear_sweep(NULL, views, num_inner_coords, mtf_direction, sweep_sign, shot_dist.shots, start_from);

    /* ks_dbg("Start from %d, num_inner_encodes = %d, num_inner_coords = %d, etl = %d, Current center encode = %d, center = %d", start_from, num_inner_encodes, num_inner_coords, shot_dist.encodes_per_shot, current_center_encode, center); */

    if (current_center_encode == center) {
      /* ks_dbg("Current center encode %d matches desired center encodes with %d linear encodes out of %d", center, num_inner_encodes, shot_dist.encodes_per_shot); */
      /* Prefer odd num_inner_encodes to avoid discontinuity in k-space center, except for the honorable linear sweep */
      if (!(num_inner_encodes % 2) &&
           (num_inner_encodes > 0) &&
           (num_inner_encodes != shot_dist.encodes_per_shot)) {
        if (ks_peplan_find_center_from_linear_sweep(NULL, views,
                                                    num_inner_coords - shot_dist.shots,
                                                    mtf_direction,
                                                    sweep_sign,
                                                    shot_dist.shots,
                                                    start_from) == current_center_encode) {
          num_inner_encodes -= 1;
          /* ks_dbg("Found odd solution with %d/%d linear encodes", num_inner_encodes, shot_dist.encodes_per_shot); */
        } else {
          /* ks_dbg("Odd solution not possible"); */
        }
      }
      break;
    }
  } /* Done search inner */


  num_inner_coords = IMin(2, num_inner_encodes * shot_dist.shots, num_coords);
  /* ks_dbg("center = %d\tnum_inner_encodes = %d\tnum_inner_coords = %d\tshot_dist.shots = %d", center, num_inner_encodes, num_inner_coords, shot_dist.shots); */
  /* Sort the inner coords  */
  if (mtf_direction == MTF_Y) {
    qsort(views, num_inner_coords, sizeof(KS_VIEW), ks_get_comp_kview_y_z(sweep_sign));
  } else {
    qsort(views, num_inner_coords, sizeof(KS_VIEW), ks_get_comp_kview_z_y(sweep_sign));
  }
  /* Assign the inner coords */
  ks_assign_encodes(views, num_inner_coords, shot_dist.readout_start + start_from, num_inner_encodes, 1);

  /* Sort rest of coords by R and assign them */
  /* Assign the outer coords - which are already sorted by abs(mtf_dir) */
  const int num_outer_coords = num_coords - num_inner_coords;
  if (num_outer_coords > 0) {
    /* Single-shot solution - This is still applicable to multi-shot but has different MTF than split-shot mode */
    if (split_shots == 0) { /* LCPO */
      int is_split_segment[] = {0, 0};
      const int num_shots = shot_dist.shots; /* CEIL_DIV(num_inner_coords, num_inner_encodes); */ /* Segment size should match linear inner segments */
      const int sign = start_from == 0 ? 1 : -1; 
      (void)sign;
      int rest_start = shot_dist.readout_start + start_from == 0 ? num_inner_encodes
                                                                 : (start_from - 1);
      KS_VIEW* outer_views = views + num_inner_coords;
      /* Assign encode segments pair-wise radially until there are no remaining coordinates */
      int rem_outer_coords = num_outer_coords;
      while (rem_outer_coords > 0) {
        KS_VIEW* next_pair = outer_views + num_outer_coords - rem_outer_coords;

        const int next_pair_size = IMin(2, 2 * num_shots, rem_outer_coords);

        /* Sort along MTF (Y / Z) ascendingly */
          if (mtf_direction == MTF_Y) { 
          qsort(next_pair, next_pair_size, sizeof(KS_VIEW), ks_get_comp_kview_y_z(1));
        } else {
          qsort(next_pair, next_pair_size, sizeof(KS_VIEW), ks_get_comp_kview_z_y(1));
        }


        int num_first = IMin(2, num_shots, rem_outer_coords);
        rem_outer_coords -= num_first;
        int num_second = IMin(2, num_shots, rem_outer_coords);
        rem_outer_coords -= num_second;


        if (mtf_direction == MTF_Y) { 
          is_split_segment[0] = next_pair[0].coord->y         * next_pair[num_first              - 1].coord->y < 0;
          if (num_second) {
            is_split_segment[1] = next_pair[num_first].coord->y * next_pair[num_first + num_second - 1].coord->y < 0;
          }
        } else {
          is_split_segment[0] = next_pair[0].coord->z         * next_pair[num_first              - 1].coord->z < 0;
          if (num_second) {
            is_split_segment[1] = next_pair[num_first].coord->z * next_pair[num_first + num_second - 1].coord->z < 0;
          }
        }

        if (is_split_segment[0]) {
          ks_assign_encodes(next_pair,              num_first, rest_start++, 1, 1);
          ks_assign_encodes(next_pair + num_first, num_second, rest_start++, 1, 1);
        } else if (is_split_segment[1]) {
          ks_assign_encodes(next_pair + num_first, num_second, rest_start++, 1, 1);
          ks_assign_encodes(next_pair,              num_first, rest_start++, 1, 1);
        } else { /* Both segments are full */
          /* On same side? */
          int same_side = 0;
          if (mtf_direction == MTF_Y) { 
            same_side = next_pair[0].coord->y         * next_pair[num_first + num_second - 1].coord->y > 0;
          } else {
            same_side = next_pair[0].coord->z         * next_pair[num_first + num_second - 1].coord->z > 0;
          }
          if (same_side) {
            if (mtf_direction == MTF_Y) {
              qsort(next_pair, num_first+num_second, sizeof(KS_VIEW), &ks_comp_kview_dist_y_z);
            } else {
              qsort(next_pair, num_first+num_second, sizeof(KS_VIEW), &ks_comp_kview_dist_z_y);
            }
            ks_assign_encodes(next_pair            ,   num_first, rest_start++, 1, 1);
            ks_assign_encodes(next_pair + num_first,  num_second, rest_start++, 1, 1);
          } else { /* On both sides */
            if (sweep_sign == 1) { /* low-to-high */
              ks_assign_encodes(next_pair,              num_first, rest_start++, 1, 1);
              ks_assign_encodes(next_pair + num_first, num_second, rest_start++, 1, 1);
  /*             qsort(next_pair, num_first+num_second, sizeof(KS_VIEW), ks_get_comp_kview_y_z(sweep_sign)); */
            } else { /* high-to-low */
              ks_assign_encodes(next_pair + num_first, num_second, rest_start++, 1, 1);
              ks_assign_encodes(next_pair,              num_first, rest_start++, 1, 1);
  /*             qsort(next_pair, num_first+num_second, sizeof(KS_VIEW), ks_get_comp_kview_y_z(sweep_sign)); */
          }
          }
        }

      }
    } else {  /* Split-shot solution */
      if (mtf_direction == MTF_Y) {
        qsort(views + num_inner_coords, num_outer_coords, sizeof(KS_VIEW), &ks_comp_kview_dist_y_z);
      } else {
        qsort(views + num_inner_coords, num_outer_coords, sizeof(KS_VIEW), &ks_comp_kview_dist_z_y);
      }

      ks_assign_encodes(views + num_inner_coords,
                        num_outer_coords,
                        shot_dist.readout_start + start_from == 0 ? num_inner_encodes
                                                                  : (start_from - 1),
                        shot_dist.encodes_per_shot - num_inner_encodes,
                        start_from == 0 ? 1 : -1);
    }
  } else {
    /* ks_dbg("No pivot coords"); */
  }
  return SUCCESS;
}

  STATUS ks_generate_peplan_from_kcoords_boustrophedon(const KS_KSPACE_ACQ* kacq, KS_PHASEENCODING_PLAN* peplan, const int etl, const KS_MTF_DIRECTION mtf_direction) {
  if (kacq == NULL || peplan == NULL) {
    return KS_THROW("NULL inputs");
  }
  if (kacq->num_coords <= 0 || kacq->coords == NULL) {
    return KS_THROW("No coordinates");
  }
  if (etl <= 0) {
    return KS_THROW("etl must be > 0");
  }

  const int num_coords = kacq->num_coords;
  /* Sort all coords by the outer dimension (ascending).
      MTF_Y: outer = kz, inner = ky  (meander along ky, step in kz)
      MTF_Z: outer = ky, inner = kz  (meander along kz, step in ky) */
  if (mtf_direction == MTF_Y) {
    qsort(kacq->coords, num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_z_y);
  } else {
    qsort(kacq->coords, num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_y_z);
  }

  /* Re-sort each group (constant outer value) alternating ascending/descending.
     Even groups are already ascending from step 1; only odd groups need re-sorting. */
  int idx = 0;
  int group_idx = 0;
  while (idx < num_coords) {
    const int outer_val = (mtf_direction == MTF_Y) ? kacq->coords[idx].z : kacq->coords[idx].y;
    int group_end = idx + 1;
    while (group_end < num_coords) {
      const int cur_outer = (mtf_direction == MTF_Y) ? kacq->coords[group_end].z
                                                     : kacq->coords[group_end].y;
      if (cur_outer != outer_val) break;
      group_end++;
    }
    if (group_idx % 2 != 0) {
      const int group_size = group_end - idx;
      if (mtf_direction == MTF_Y) {
        qsort(kacq->coords + idx, group_size, sizeof(KS_KCOORD), ks_comp_kcoord_y_z_descend);
      } else {
        qsort(kacq->coords + idx, group_size, sizeof(KS_KCOORD), ks_comp_kcoord_z_y_descend);
      }
    }
    idx = group_end;
    group_idx++;
  }

  /* Build views with sequential encode assignment */
  KS_VIEW* views = (KS_VIEW*) malloc(num_coords * sizeof(KS_VIEW));
  if (views == NULL) {
    return KS_THROW("malloc failed");
  }
  KS_PEPLAN_SHOT_DISTRIBUTION shot_dist = ks_peplan_distribute_shots(etl, num_coords, 0);
  ks_assign_views_to_contiguous_shots(views, kacq->coords, num_coords, &shot_dist);

  /* Allocate and fill peplan */
  const int shots = shot_dist.shots;
  STATUS s = ks_phaseencoding_alloc(peplan, etl, shots);
  if (s != SUCCESS) {
    free(views);
    return s;
  }
  s = ks_peplan_assign_encodes(peplan, views, num_coords, &shot_dist, 0,
                              kacq->matrix_size[YGRAD] / 2,
                              kacq->matrix_size[ZGRAD] / 2, 1);
  free(views);
  return s;
}

STATUS ks_generate_peplan_from_kcoords_spiral(const KS_KSPACE_ACQ* kacq,
                                              KS_PHASEENCODING_PLAN* peplan,
                                              const int etl,
                                              const KS_SPIRAL_DIRECTION spiral_direction) {
  if (kacq == NULL || peplan == NULL) {
    return KS_THROW("NULL inputs");
  }
  if (kacq->num_coords <= 0 || kacq->coords == NULL) {
    return KS_THROW("No coordinates");
  }
  if (etl <= 0) {
    return KS_THROW("etl must be > 0");
  }

  const int num_coords = kacq->num_coords;
  KS_PEPLAN_SHOT_DISTRIBUTION shot_dist = ks_peplan_distribute_shots(etl, num_coords, 0);
  const int shots = shot_dist.shots;

    /* Recompute polar coordinates from the grid.
      Radius is normalised s.t. the outermost encodes are at radius 1, regardless of the matrix size.*/
  const float TWO_PI = 2.0f * (float)M_PI;
  const float y_extent = kacq->matrix_size[YGRAD] > 1 ? (float)kacq->matrix_size[YGRAD] / 2.0f : 1.0f;
  const float z_extent = kacq->matrix_size[ZGRAD] > 1 ? (float)kacq->matrix_size[ZGRAD] / 2.0f : 1.0f;
  const float min_extent = y_extent < z_extent ? y_extent : z_extent;
  for (int idx = 0; idx < num_coords; idx++) {
    const float y = (float)kacq->coords[idx].y + 0.5f;
    const float z = (float)kacq->coords[idx].z + 0.5f;
    const float y_norm = y / y_extent;
    const float z_norm = z / z_extent;
    kacq->coords[idx].r = sqrtf(y_norm * y_norm + z_norm * z_norm);
    float t = atan2(z, y);
    if (t < 0.0f) t += TWO_PI;
    kacq->coords[idx].t = t;
  }

  /* We do a radar-like sweep annulus by annulus. Coordinates are first ordered by
     radius so each annulus is contiguous, then swept by theta inside that annulus. */
  if (spiral_direction == SPIRAL_OUT) {
    qsort(kacq->coords, num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_r_t);
  } else {
    qsort(kacq->coords, num_coords, sizeof(KS_KCOORD), ks_comp_kcoord_r_t_descend);
  }

  int group_start = 0;
  /* Group into annuli */
  const float annulus_width_pixels = 1.25f;
  const float annulus_width = annulus_width_pixels / min_extent;
  while (group_start < num_coords) {
    const int annulus = (int)floorf(kacq->coords[group_start].r / annulus_width);
    int group_end = group_start + 1;
    while (group_end < num_coords && (int)floorf(kacq->coords[group_end].r / annulus_width) == annulus) {
      group_end++;
    }

    if (spiral_direction == SPIRAL_OUT) {
      qsort(kacq->coords + group_start, group_end - group_start, sizeof(KS_KCOORD), ks_comp_kcoord_t_r);
    } else {
      qsort(kacq->coords + group_start, group_end - group_start, sizeof(KS_KCOORD), ks_comp_kcoord_t_r_descend);
    }

    group_start = group_end;
  }

  /* Assign views */
  KS_VIEW* views = (KS_VIEW*) malloc(num_coords * sizeof(KS_VIEW));
  if (views == NULL) {
    return KS_THROW("malloc failed");
  }
  ks_assign_views_to_contiguous_shots(views, kacq->coords, num_coords, &shot_dist);

  STATUS s = ks_phaseencoding_alloc(peplan, etl, shots);
  if (s != SUCCESS) {
    free(views);
    return s;
  }
  s = ks_peplan_assign_encodes(peplan, views, num_coords, &shot_dist, 0,
                               kacq->matrix_size[YGRAD] / 2, kacq->matrix_size[ZGRAD] / 2, 1);
  free(views);
  return s;
}

STATUS ks_generate_peplan_from_kcoords(const KS_KSPACE_ACQ* kacq,
                                       KS_PHASEENCODING_PLAN* peplan,
                                       KS_MTF_DIRECTION mtf_direction,
                                       KS_VIEW_SHOT_ORDER encode_shot_assignment_order,
                                       KS_VIEW_MTF_PROFILE mtf_profile,
                                       const int etl /* echoes per shot */,
                                       int center /* Center echo in k-space [0, etl-1]*/,
                                       const KS_COORDINATE_TYPE * coord_types ) {
  if (center == KS_NOTSET) {
      mtf_profile = LINEAR_SWEEP;
      KS_PEPLAN_SHOT_DISTRIBUTION tmp_dist = ks_peplan_distribute_shots(etl, kacq->num_coords, center);
      const int center_lth_linsweep = ks_peplan_find_center_from_linear_sweep(kacq->coords, NULL, kacq->num_coords, mtf_direction, +1 /* lth */, tmp_dist.shots, 0 /* start_from */);
      const int center_htl_linsweep = ks_peplan_find_center_from_linear_sweep(kacq->coords, NULL, kacq->num_coords, mtf_direction, -1 /* htl */, tmp_dist.shots, 0 /* start_from */);
      if (center_lth_linsweep != center_htl_linsweep) {
        return KS_THROW("Ambigous center encode: center encode was not set and partial Fourier coords are supplied");
      }
      center = center_lth_linsweep;
  } else if (center < 0) {
    return KS_THROW("Center index cannot be negative (requested %d)", center);
  }

  if (center >= etl) {
    return KS_THROW("Center index must be in [0,etl-1] (center=%d, etl=%d)", center, etl);
  }
  if (kacq->num_coords <= 0) {
    return KS_THROW("num_coords is %d", kacq->num_coords);
  }

  if (kacq->coords == NULL) {
    return KS_THROW("No coordinates supplied");
  }

  if (center >= kacq->num_coords) {
    return KS_THROW("Center index must be in [0,num_coords-1] (center=%d, num_coords=%d)", center, kacq->num_coords);
  }

  int idx, readout, shot;
  KS_KCOORD* K = kacq->coords;
  /* Check if K.z is the same for all KCOORDS. If it is then we don't set it later. TODO: Use kacq->matris[ZGRAD]*/
  int is3d = 0;
  for (idx = 0; idx < kacq->num_coords && !is3d; idx++) {
    is3d |= (K[0].z != K[idx].z);
  }

  if (is3d == 1) {
    if (mtf_profile == CHEVRON || mtf_profile == ECLIPSE) {
      STATUS s;
      s = ks_generate_peplan_from_kcoords_american_3d(kacq, peplan, mtf_profile, etl, center); /* Note that there is no mtf_direction here.  */
      return s;
    }
    if (mtf_profile == CROC) {
      STATUS s;
      s = ks_generate_peplan_from_kcoords_CROC(kacq, peplan, etl, center);
      return s;
    }
    if (mtf_profile == BOUSTROPHEDON) {
      STATUS s;
      s = ks_generate_peplan_from_kcoords_boustrophedon(kacq, peplan, etl, mtf_direction);
      return s;
    }
    if (mtf_profile == SPIRAL) {
      STATUS s;
      KS_SPIRAL_DIRECTION spiral_direction = mtf_direction == MTF_R_REVERSED ? SPIRAL_IN : SPIRAL_OUT;
      s = ks_generate_peplan_from_kcoords_spiral(kacq, peplan, etl, spiral_direction);
      return s;
    }
  }

  if (is3d == 0) {
    if (mtf_profile == SPLIT_SHOTS_AMERICAN) {
      return ks_generate_peplan_from_kcoords_american_2d(kacq, peplan, mtf_direction, etl, center);
    } if (mtf_profile == SPLIT_SHOTS_DUTCH) {
      return ks_generate_peplan_from_kcoords_dutch(kacq, peplan, mtf_direction, etl, center);
    }
  }  

  KS_PEPLAN_SHOT_DISTRIBUTION shot_dist = ks_peplan_distribute_shots(etl, kacq->num_coords, center);



  if ((is3d == 0) && ((mtf_direction == MTF_R) || (mtf_direction == MTF_R))) {
    return KS_THROW("Radial MTF is not supported for 2D acquisitions");
  }

  KS_COORDINATE_TYPE default_coord_types[] = {CARTESIAN_COORD, CARTESIAN_COORD};
  if (coord_types == NULL) {
    coord_types = default_coord_types;
  }

  /* Create views */
  KS_VIEW* views = (KS_VIEW*) malloc(kacq->num_coords * sizeof(KS_VIEW));

  for (idx = 0; idx < kacq->num_coords; idx++) {
    views[idx].coord = &(K[idx]);
    views[idx].encode = KS_NOTSET;
    views[idx].shot = KS_NOTSET;
  }

  /*
   Calculate the view order (e.g. center out if center=0) using a pivot algorithm. 
   The view order describes the T2 modulation in k-space.
   If you want to implement linear sweep, view_order is simply [0, 1, ..., shot_dist.encodes_per_shot-1] 
  */
  int view_order[shot_dist.encodes_per_shot];
  memset(view_order, KS_NOTSET, sizeof(view_order));
  view_order[0] = KS_NOTSET;
  if ((mtf_direction == MTF_Y) || (mtf_direction == MTF_Z)) {
    switch (mtf_profile) {
    case PIVOT_CENTER_OUTER_LINEAR: {
      ks_pivot_specific_center_symk(view_order, shot_dist.encodes_per_shot, center);
      break;
    } case SPLIT_SHOTS_LCPO:
      case LCPO: {
        if (coord_types[0] == RADIAL_COORD || coord_types[1] == RADIAL_COORD) {
         ks_pivot_linear_center_symk(view_order, shot_dist.encodes_per_shot, center);
        } else {
          ks_lcpo_assign_encodes(views, kacq->num_coords, mtf_direction,
                                mtf_profile == SPLIT_SHOTS_LCPO ? 1 : 0,
                                etl, center);
        }
      break;
    } case LINEAR_ROLL: {
      ks_pe_linear_roll(view_order, shot_dist.encodes_per_shot, center);
      break;
    } case PIVOT_ROLL: {
      ks_pe_pivot_roll(view_order, shot_dist.encodes_per_shot, center);
      break;
    } case LINEAR_SWEEP: {
      ks_pe_linear(view_order, shot_dist.encodes_per_shot);
      break;
    }
    default:
      free(views);
      return KS_THROW("MTF profile %d not implemented", (int)mtf_profile);
      break;
    }
  } else if ((mtf_direction == MTF_R) || (mtf_direction == MTF_T)) {
/*     ks_pivot_specific_center_radk(view_order, shot_dist.encodes_per_shot, center); */
    ks_pe_permute_pclo_rad(view_order, shot_dist.encodes_per_shot, center);

  }

  if (view_order[0] != KS_NOTSET) {
    /* Permute has been set - assign encodes */
    /* Sort KCOORDS according to their radius or ky coordinate */
    switch (mtf_direction) {
      case MTF_R: {
        qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_r_t);
        break;
      }
      case MTF_T: {
        qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_t_r);
        break;
      }
      case MTF_Y: {
        qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_y_z);
        break;
      }
      case MTF_Z: {
        qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_z_y);
        break;
      }
      default:
      {
        return ks_error("Unknown mtf_direction: %i", mtf_direction);
      }
    }

    /* Assign each point an encode ("echo" in an echo train) according to the desired view order (center out etc)
    The linear indexing (idx) is a consequence of the sort above */
    const int readout_end = shot_dist.readout_start + shot_dist.encodes_per_shot;
    for (idx = 0, readout = shot_dist.readout_start; readout < readout_end; readout++) {
      for (shot = 0; shot < shot_dist.shots; shot++) {
        int block = shot < shot_dist.first_blocks ? 0 : 1;
        if (block == 1 && readout == (readout_end -1)) {
          continue;
        }
        if (idx < kacq->num_coords) {
          views[idx].encode = view_order[readout];
        }
        idx++;
      }
    } 
  }

  /* Now each point has an encode (or TE if you prefer). It's time to sort the encodes (TE's) for the desired order. */
  switch (encode_shot_assignment_order) {
    case T_R:
    {
      /* Sort for encode->theta then r. This enforces a spoke in ky-kz when we later pick encodes in a shot. 
       Note that radial is not at all related to elliptical k-space coverage. */
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_tr);
      break;
    }
    case R_T:
    {
      /* Sort for encode->r then theta. This enforces a ring in ky-kz when we later pick encodes in a shot. */
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_rt);
      break;
    }
    case Y_Z:
    {
      /* Sort for encode->kz. This enforces a column in ky-kz when we later pick encodes in a shot */
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_yz);
      break;
    }
    case Z_Y:
    {
      /* Sort for encode->ky. This enforces a row in ky-kz when we later pick encodes in a shot */
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_zy);
      break;
    }

    case Y_R:
    {
      /* Sort for encode->kz. This enforces a column in ky-kz when we later pick encodes in a shot */
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_yr);
      break;
    }
    case Z_R:
    {
      /* Sort for encode->ky. This enforces a row in ky-kz when we later pick encodes in a shot */ 
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_zr);
      break;
    }

    case SEC_T:
    {
      qsort(views, kacq->num_coords, sizeof(KS_VIEW), &ks_comp_kview_e_sec_t);
      break;
    }

    default:
    {
      return ks_error("Unknown encode_shot_assignment_order: %i", encode_shot_assignment_order);
    }
  }

  /* Make the actual phase encoding plan */
  STATUS status = ks_phaseencoding_alloc(peplan, etl, shot_dist.shots);
  KS_RAISE(status);

  int yshift = coord_types[0] == CARTESIAN_COORD ? kacq->matrix_size[YGRAD]/2 : 0;
  int zshift = coord_types[1] == CARTESIAN_COORD ? kacq->matrix_size[ZGRAD]/2 : 0;

  status = ks_peplan_assign_shots(peplan, views, kacq->num_coords, &shot_dist, 0, yshift, zshift, is3d);
  free(views);

  KS_RAISE(status);

  return SUCCESS;
}




/* 
  The aim for this function is to create a continous 2d kspace with a ky width of etl for parallel imaging calibration purposes 
  The fully sampled area of kspace (width = etl) is placed in the center of the original kspace
*/
STATUS ks_phaseencoding_generate_2d_singleshot_cal(KS_PHASEENCODING_PLAN *phaseenc_plan_ptr, int yres){
  STATUS status;
  const int minimum_etl = 6;

  const int etl = phaseenc_plan_ptr->encodes_per_shot;

  if (etl < minimum_etl) {
    return ks_error("%s: etl must be >= %d", __FUNCTION__, minimum_etl);
  }

  /* allocate KS_PHASEENCODING_PLAN table (all entries will be initialized to KS_NOTSET) */
  status = ks_phaseencoding_alloc(phaseenc_plan_ptr, etl, 1);
  KS_RAISE(status);

  const int step = 1; /* Could be set to -1 too. We had logic to choose before. */

  const float ky_center_main = (yres-1)/2.0f;
  const float ky_center_cal = (etl-1)/2.0f;
  int i = 0;
  int ky = ky_center_main - step * ky_center_cal;
  if (ky < 0) {
    ky = 0;
  }
  for (; i < etl && i < yres; i++){
    ks_phaseencoding_set(phaseenc_plan_ptr, i, -1, ky, KS_NOTSET);
    ky += step;
  }
  /* In case yres < etl */
  for (i = yres; i < etl; i++){
    ks_phaseencoding_set(phaseenc_plan_ptr, i, -1, KS_NOTSET, KS_NOTSET);
  }

  return SUCCESS;
}




/* Assumes the cal region always starts with an acquired line: */
/* comes from - RDN_FACTOR((N - cal) / 2, R) */
int ks_cal_from_nacslines(int R, int nacslines) {
  if (R == 1) return nacslines;
  if (nacslines <= 0) return 0;
  return nacslines + nacslines / (R - 1) + 1;
}




STATUS ks_add_3d_coords_radially(KS_KSPACE_ACQ* kacq, const int num_to_add) {
    int Ny = kacq->matrix_size[YGRAD];
    int Nz = kacq->matrix_size[ZGRAD];
    int i = 0;
    int y = 0;
    int z = 0;

    uint8_t* occ = (uint8_t*)calloc(Ny * Nz, 1);
    if (!occ) return KS_THROW("alloc failed");

    /* Build occupancy */
    for (i = 0; i < kacq->num_coords; i++) {
        y = (int)(kacq->coords[i].y + Ny/2);
        z = (int)(kacq->coords[i].z + Nz/2);

        if (y >= 0 && y < Ny && z >= 0 && z < Nz) {
            occ[z * Ny + y] = 1;
        }
    }

    /* Generate candidates */
    int max_pts = Ny * Nz;
    KS_KCOORD* cand = (KS_KCOORD*)malloc(max_pts * sizeof(KS_KCOORD));
    if (!cand) {
        free(occ);
        return KS_THROW("alloc failed");
    }

    int n_cand = 0;

    float y_max = Ny / 2.0f;
    float z_max = Nz / 2.0f;

    for (z = 0; z < Nz; z++) {
        for (y = 0; y < Ny; y++) {

            if (occ[z * Ny + y]) continue;

            float y1 = (Ny > 1) ? (y - Ny/2 + 0.5f) / y_max : 0.f;
            float z1 = (Nz > 1) ? (z - Nz/2 + 0.5f) / z_max : 0.f;

            cand[n_cand].y = y - Ny/2;
            cand[n_cand].z = z - Nz/2;
            cand[n_cand].r = sqrt(y1*y1 + z1*z1);
            cand[n_cand].t = atan2(z1, y1);

            n_cand++;
        }
    }

    qsort(cand, n_cand, sizeof(KS_KCOORD), ks_comp_kcoord_r_t);

    /* Resize kacq */
    KS_KCOORD* K = (KS_KCOORD*)realloc(kacq->coords, (kacq->num_coords + num_to_add) * sizeof(KS_KCOORD));
    if (!K) {
        free(occ);
        free(cand);
        return KS_THROW("Failed reallocation");
    }
    kacq->coords = K;

    /* Add */
    K = kacq->coords + kacq->num_coords;
    for (i = 0; i < n_cand && i < num_to_add; i++) {
        *K++ = cand[i];
    }

    kacq->num_coords += i;

    free(occ);
    free(cand);

    if (i < num_to_add) {
        return KS_THROW("Could only add %d coordinates out of requested %d", i, num_to_add);
    }

    return SUCCESS;
}

STATUS ks_generate_3d_coords_caipi(KS_KSPACE_ACQ* kacq,
                                   int Ny, int Nz,
                                   int nover_y, int nover_z,
                                   int Ry, int Rz,
                                   int cal_y, int cal_z,
                                   KS_COVERAGE cal_coverage,
                                   KS_COVERAGE acq_coverage,
                                   /* CAIPIRINHA related */
                                   int dy, int dz,
                                   int step_y, int step_z) {
(void)(dz);
(void)(step_z);
if (Nz == KS_NOTSET) {
  Nz = 1;
}


KS_PF_EARLYLATE pf_ymode = KS_PF_NO;
KS_PF_EARLYLATE pf_zmode = KS_PF_NO;


if (nover_y < 0) {
  pf_ymode = KS_PF_EARLY; /* Remove low ky */
} else if (nover_y > 0) {
  pf_ymode = KS_PF_LATE; /* Remove high ky */
}

if (nover_z < 0) {
  pf_zmode = KS_PF_EARLY; /* Remove low kz */
} else if (nover_z > 0) {
  pf_zmode = KS_PF_LATE; /* Remove high kz */
}

nover_y = abs(nover_y);
nover_z = abs(nover_z);

if (Nz == 1) {
  pf_zmode = KS_PF_NO;
  cal_coverage = RECTANGULAR;
  acq_coverage = RECTANGULAR;
  cal_z = 1;
  dy = 0;
  Rz = 1;
}


KS_KCOORD* K = (KS_KCOORD*)realloc(kacq->coords, (Nz*Ny/(Ry*Rz) + 2*cal_y*cal_z) * sizeof(KS_KCOORD));
if (!K) {
  return KS_THROW("Failed reallocation");
}


kacq->coords = K;
kacq->num_coords = 0;

int y = 0;
int z = 0;
int row, col;
int offset = 0;
int entry = 0;
const float epsilon = 1e-6f; /* Floating-point roundoff error */


/* Generate acq coordinates honoring acceleration. Cal region is excluded */
cal_z = IMax(2, cal_z, 1);

double center_y = Ny / 2.0;
double center_z = Nz / 2.0;
int y_cal_low = (int)floor(center_y - cal_y / 2.0);
int y_cal_hi  = y_cal_low + cal_y;
int z_cal_low = (int)floor(center_z - cal_z / 2.0);
int z_cal_hi  = z_cal_low + cal_z;

float y_max = Ny / 2.0;
float z_max = Nz / 2.0;
int skipped = 0;

for (row = 0; row < Nz; row += Rz) {
    z = row;

    /* PF z */
    if ((pf_zmode == KS_PF_LATE  && z >= (Nz/2 + nover_z)) ||
        (pf_zmode == KS_PF_EARLY && z <  (Nz/2 - nover_z))) { continue; }

    for (col = 0; col < Ny; col += Ry) {
      y = col + offset;
      if ((pf_ymode == KS_PF_LATE  && y >= (Ny/2 + nover_y)) ||
          (pf_ymode == KS_PF_EARLY && y <  (Ny/2 - nover_y))) { continue; }

      if (y >= Ny) {
         continue;
      }
      double y1 = Ny > 1 ? (y - Ny/2 + .5) / y_max /* Normalized global coordinate [-1, 1] */
                         : 0.0;
      double z1 = Nz > 1 ? (z - Nz/2 + .5) / z_max /* Normalized global coordinate [-1, 1] */
                         : 0.0;
      KS_KCOORD c = {y - Ny/2,
                     z - Nz/2,
                     (float)sqrt(y1*y1 + z1*z1),
                     (float)atan2(z1, y1)
                     };
     if (acq_coverage == ELLIPTICAL && c.r > 1.0 + epsilon) { /* Floating-point roundoff (might be exactly on circumference) */
       continue;
     }
     if (Ry > 1 || Rz > 1) {
       if (y >= y_cal_low &&
           y < y_cal_hi &&
           z >= z_cal_low &&
           z < z_cal_hi) {
          if (cal_coverage == RECTANGULAR) { 
            skipped++;
            continue;
          }
          double y_dist_from_center = (y + 0.5 - Ny / 2.0);
          double z_dist_from_center = (z + 0.5 - Nz / 2.0);

          /* Local coordinates (local as in cal region) */
          double l_y = y_dist_from_center / (cal_y / 2.0);
          double l_z = z_dist_from_center / (cal_z / 2.0);
          double l_r = l_y * l_y + l_z * l_z; /* Local, no need for sqrt here as it's only checked against unity */
          if (l_r < 1.0 + epsilon) {
            skipped++;
            continue;
          }
        }
     }
    K[entry++] = c;
    }
    offset += dy;
    offset = offset % (step_y + 1);
}

/* Fill cal region */
if (Ry > 1 || Rz > 1) {
  for (y = y_cal_low; y < y_cal_hi; y++) {
    for (z = z_cal_low; z < z_cal_hi; z++) {
      if (z < 0 || z >= Nz || y < 0 || y >= Ny) {
        KS_THROW("Cal region is outside the accelerated region - (y,z) = (%d,%d)", y, z);
        continue;
      }

      if (cal_coverage == ELLIPTICAL) {
        double y_dist_from_center = (y + 0.5 - Ny / 2.0);
        double z_dist_from_center = (z + 0.5 - Nz / 2.0);
        double l_y = y_dist_from_center / (cal_y / 2.0);
        double l_z = z_dist_from_center / (cal_z / 2.0);
        double l_r = l_y * l_y + l_z * l_z;

        if (l_r > 1.0 + epsilon) {
          continue;
        }
      }

      double y1 = Ny > 1 ? (y - Ny/2 + 0.5) / y_max : 0;
      double z1 = Nz > 1 ? (z - Nz/2 + 0.5) / z_max : 0;

      KS_KCOORD c = {
        y - Ny/2,
        z - Nz/2,
        (float)sqrt(y1*y1 + z1*z1),
        (float)atan2(z1, y1)
      };

      K[entry++] = c;
    }
  }
}

kacq->num_coords = entry;
kacq->matrix_size[YGRAD] = Ny;
kacq->matrix_size[ZGRAD] = Nz;
return SUCCESS;
}  /*ks_generate_3d_coords_caipi*/




STATUS ks_generate_3d_coords_simple(KS_KSPACE_ACQ* kacq,
                                    int Ny, int Nz,
                                    int nover_y, int nover_z,
                                    int Ry, int Rz,
                                    int cal_y, int cal_z,
                                    KS_COVERAGE cal_coverage,
                                    KS_COVERAGE acq_coverage) {
  return ks_generate_3d_coords_caipi(kacq, Ny, Nz, nover_y, nover_z, Ry, Rz, cal_y, cal_z, cal_coverage, acq_coverage, 0, 0, 0, 0);
}/* ks_generate_3d_coords_simple */




STATUS ks_generate_2d_coords_cal(KS_KSPACE_ACQ* kacq, int num_desired_coords, int yres) {
  int max_y = RUP_FACTOR(num_desired_coords, 2);

  STATUS status;

  status = ks_generate_3d_coords_simple(kacq, max_y, KS_NOTSET, 0, 0, 1, 1, 0, 0, RECTANGULAR, RECTANGULAR);
  KS_RAISE(status);

  kacq->matrix_size[YGRAD] = yres;
  if (kacq->num_coords < num_desired_coords) {
    return KS_THROW("Could not generate enough cal coords (%d < %d)", kacq->num_coords, num_desired_coords);
  }

  /* Make sure we get the most central coordinates */
  qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), &ks_comp_kcoord_dist_y_z);

  if (kacq->num_coords > num_desired_coords) {
    KS_THROW("kacq->num_coords > num_desired_coords (%d > %d). Expected if num_desired_coords is odd", kacq->num_coords, num_desired_coords);
    kacq->num_coords = num_desired_coords;
    KS_KCOORD* tmp = (KS_KCOORD*)realloc(kacq->coords, kacq->num_coords);
    if (tmp) {
      kacq->coords = tmp;
    } else {
      return KS_THROW("Reallocation failed");
    }
  }
  qsort(kacq->coords, kacq->num_coords, sizeof(KS_KCOORD), &ks_comp_kcoord_y_z);
  return SUCCESS;
}




STATUS ks_generate_3d_coords_radial(KS_KSPACE_ACQ* kacq,
                                    int tiny_level,
                                    int Ny, int Nz,
                                    int Ry, int Rz,
                                    KS_RADIAL_SAMPLING_MODE radial_mode,
                                    KS_RADIAL_SAMPLING_COVERAGE radial_sampling_coverage)  {

int entry = 0;
double phi = 0;
double phi_step;
double delta_phi;
double theta = 0;
double theta_step;
double xx, yy, zz;
int kz = 0;
int spoke = 0;

/* Cartesian */
if(radial_mode == CARTESIAN){ 
  return ks_generate_3d_coords_simple(kacq, Ny, Nz, 0, 0, Ry, Rz, 0, 0,  RECTANGULAR, RECTANGULAR);
}




/* Through center (2d or kooshball) */
else if(radial_mode == RADIAL_THROUGH_CENTER){ 

  kacq->num_coords = ceil(fabs(Ny * Nz * PI/2.0 / Ry / Rz)) ;

  KS_KCOORD* K = (KS_KCOORD*)realloc(kacq->coords, kacq->num_coords * sizeof(KS_KCOORD));
  if (!K) {
    return KS_THROW("Failed (re)allocation");
  }
  kacq->coords = K;
  for (entry = 0; entry < kacq->num_coords; entry++) {

    if(Nz<=1){ /* 2D radial */
      if(tiny_level==0){
        delta_phi = 180.0 / kacq->num_coords;
        phi = entry*delta_phi;
        theta = 0;
      }
      else{
        phi_step =  1.0 /(KS_GOLDEN_MEANS_1D+tiny_level-1);
        phi = 180.0 * (phi_step * entry - floor(phi_step * entry));
        theta = 0;
      }
      if (radial_sampling_coverage == SAMPLE_360 && entry%2){ /* cover full circle */
        phi = phi + 180;
      }
      kacq->coord_type[0] = RADIAL_COORD;
    }
    else{ /* Kooshball */
      if(tiny_level==0){
        zz = fabs((double)entry - (double) kacq->num_coords-1.0)/ (double) kacq->num_coords;
        xx = cos(pow( kacq->num_coords*PI,0.5)*asin(zz))*pow(1-pow(zz,2),0.5);
        yy = sin(pow( kacq->num_coords*PI,0.5)*asin(zz))*pow(1-pow(zz,2),0.5);

        theta = 90.0-(acos(zz) * 180.0 / PI);
        phi = ((yy > 0) - (yy < 0))*acos(xx/pow(pow(xx,2)+pow(yy,2),0.5))* 180.0 / PI;

      }
      else{
        phi_step = KS_GOLDEN_MEANS_2D_2;
        phi_step =  1.0/(pow(KS_GOLDEN_MEANS_2D_2,-1) + (double) tiny_level-1.0);
        phi = 360.0 * (phi_step * entry - floor(phi_step * entry));
        theta_step = phi_step/(1+KS_GOLDEN_MEANS_2D_1);
        theta = asin(theta_step * entry - floor(theta_step * entry)) * 180.0 / PI ;
      }
      if (radial_sampling_coverage == SAMPLE_360 && entry%2){ /* cover full sphere */
        theta = theta + 180;
      }
      kacq->coord_type[0] = RADIAL_COORD;
      kacq->coord_type[1] = RADIAL_COORD;
    }

    KS_KCOORD c = {(s16) (phi/KS_ANGLE_PER_INT_16_MAX_180),
                   (s16) (theta/KS_ANGLE_PER_INT_16_MAX_180),
                   (float) entry,
                   (float) entry};
    K[entry]= c;
  }
}




/* Stack of stars */
else if(radial_mode == RADIAL_SOS){ 

  kacq->num_coords = ceil(Ny * PI/2.0 / Ry / Rz)* Nz ;

  KS_KCOORD* K = (KS_KCOORD*)realloc(kacq->coords, kacq->num_coords * sizeof(KS_KCOORD));
  if (!K) {
    return KS_THROW("Failed (re)allocation");
  }
  kacq->coords = K;
  for (entry = 0; entry < kacq->num_coords; entry++) {
    if(tiny_level==0){
      delta_phi = 180.0 / ceil(Ny * PI/2.0 / Ry / Rz);
      phi = spoke*delta_phi;
    }
    else{
      phi_step =  1.0 /(KS_GOLDEN_MEANS_1D+tiny_level-1);
      phi = 180.0 * (phi_step * spoke - floor(phi_step * spoke));
    }

    if (radial_sampling_coverage == SAMPLE_360 && spoke%2){ /* cover full circle */
      phi = phi + 180;
    }

    KS_KCOORD c = {(s16) (phi/KS_ANGLE_PER_INT_16_MAX_180),
                   (s16) (kz - Nz/2),
                   (float) spoke,
                   (float) spoke};
    K[entry]= c;

    if(kz < Nz - 1){
      kz++;
    }
    else{
      kz = 0;
      spoke++;
    }
  }
  kacq->coord_type[0] = RADIAL_COORD;
  kacq->coord_type[1] = CARTESIAN_COORD;
}

else{
  return KS_THROW("Requested radial mode (%d) does not exist.", radial_mode);
}

kacq->matrix_size[YGRAD] = Ny;
kacq->matrix_size[ZGRAD] = Nz;


return SUCCESS;
} /* ks_generate_3d_coords_radial */




void set_grid(int* grid, int Nz, int y, int z) {
  grid[y*Nz + z] = 1;
}




int get_grid(int* grid, int Nz, int y, int z) {
  return grid[y*Nz + z];
}




float rand_float_in_range(float min, float max) {
  float random = (float)(rand()) / (float)RAND_MAX; /* [0.0, 1.0] */
  return (min + (random)*(max-min));
}




float ks_poisson_disc_min_spacing(const int y,
                  const int z,
                  const float center_y,
                  const float center_z,
                  const float min_spacing,
                  const float max_spacing,
                  const RADIUS_PATTERN pattern) {
  const float center_dist = sqrt( (y + 0.5 - center_y)*(y + 0.5 - center_y) +
                                  (z + 0.5 - center_z)*(z + 0.5 - center_z) );
  const float max_dist = sqrt(center_z*center_z + center_y*center_y);
  if (pattern == EXPONENTIAL) {
    return (min_spacing * exp( log(max_spacing/min_spacing) * (center_dist / max_dist) ));
  } else if (pattern == LINEAR) {
    return (min_spacing + (center_dist / max_dist)*(max_spacing - min_spacing));
  } else {
    ks_error("%s: radius pattern (%d) not implemented", __FUNCTION__, (int)pattern);
    return -1.0f;
  }
}




int check_conflicts(int* grid, int Ny, int Nz, KS_KCOORD cand, float cand_radius) {
  const int block_size = 2*(int)ks_round(cand_radius) + 1;
  const float center = block_size / 2.0;
  int y, z;
  int conflict = 0;
  for (y = 0; y < block_size; y++) {
    int ycoord = cand.y - block_size/2 + y;
    if (ycoord < 0 || ycoord >= Ny) {
      continue;
    }
    for (z = 0; z < block_size; z++) {
      int zcoord = cand.z - block_size/2 + z;
      if (zcoord < 0 || zcoord >= Nz) {
        continue;
      }

      /* Is within radius? */
      if (sqrt((y + 0.5 - center)*(y + 0.5 - center) + (z + 0.5 - center)*(z + 0.5 - center)) < cand_radius) {
        conflict |= get_grid(grid, Nz, ycoord, zcoord);
        if (conflict == 1) {
          return conflict;
        }
      }
    }
  }
  return conflict;
}




void remove_element(KS_KCOORD* coord_array, int idx, int size) {
  int i;
  for (i=idx; i < (size-1); i++) {
    coord_array[i] = coord_array[i+1];
  }
}




STATUS ks_generate_3d_coords_poisson_disc_R(KS_KSPACE_ACQ* kacq,
                                            int Ny,
                                            int Nz,
                                            float R,
                                            KS_COVERAGE cal_coverage, 
                                            int cal_y, int cal_z, 
                                            RADIUS_PATTERN pattern) {

  const float min_spacing = 1.4;
  float left_spacing = min_spacing;
  float right_spacing = IMax(2,Nz/4, Ny/4);
  STATUS status;

  status = ks_generate_3d_coords_poisson_disc(kacq, Ny, Nz, min_spacing, right_spacing, cal_coverage, cal_y, cal_z, pattern);
  KS_RAISE(status);

  const int max_iter = 10;
  float cur_R = Ny*Nz/(kacq->num_coords);
  int iter = 0;
  float cur_spacing;
  /* Binary search */
  while (fabs(cur_R - R) > 0.1 && iter < max_iter) {
    cur_spacing = (right_spacing + left_spacing) / 2.0;

    status = ks_generate_3d_coords_poisson_disc(kacq, Ny, Nz, min_spacing, cur_spacing, cal_coverage, cal_y, cal_z, pattern);
    KS_RAISE(status);

    cur_R = Ny*Nz/(float)(kacq->num_coords);


    if (cur_R > R) {
      right_spacing = cur_spacing;
    }
    if (cur_R < R) {
      left_spacing = cur_spacing;
    }

    iter++;
  }

  return SUCCESS;


}




STATUS ks_generate_3d_coords_poisson_disc(KS_KSPACE_ACQ* kacq,
                                         int Ny,
                                         int Nz,
                                         float min_spacing,
                                         float max_spacing,
                                         KS_COVERAGE cal_coverage, 
                                         int cal_y, int cal_z, 
                                         RADIUS_PATTERN pattern) {
  const int num_attempts = 50;
  const float ky_center = (Ny - 1) / 2.0;
  const float kz_center = (Nz - 1) / 2.0;
  const float ky_radius = Ny / 2.0;
  const float kz_radius = Nz / 2.0;
  const int max_queue = Ny*Nz;
  int queue_size = 0;
  int y, z;

  KS_KCOORD* queue = (KS_KCOORD*)malloc(max_queue * sizeof(KS_KCOORD));


  KS_KCOORD* K = (KS_KCOORD*)realloc(kacq->coords, Nz*Ny * sizeof(KS_KCOORD)); /* Output */
  if (!K) {
    return KS_THROW("Failed reallocation");
  }
  kacq->coords = K;
  int* grid = (int*) calloc (Ny*Nz,sizeof(int)); /* Initialized to zero */
  int idx = 0;
  int num_coords = 0;


  int y_corners[4] = {0,    0, Ny-1, Ny-1};
  int z_corners[4] = {0, Nz-1, Nz-1,    0};

  for (idx = 0; idx < 4; idx++) {
    y = y_corners[idx];
    z = z_corners[idx];
    float y2 = (y + .5 - ky_center) / ky_radius;  /* Global coordinate */
    float z2 = (z + .5 - kz_center) / kz_radius;  /* Global coordinate */
    KS_KCOORD coord = {y, z, (float)sqrt((y2 * y2) + (z2 * z2)), (float)atan2(z2, y2)};
    queue[queue_size++] = coord;
    K[num_coords++] = coord;
    set_grid(grid, Nz, coord.y, coord.z);
  }


  /* Fully sampled region */
  int y_cal_low = (Ny - cal_y) / 2;
  int y_cal_hi = y_cal_low + cal_y;
  int z_cal_low = (Nz - cal_z) / 2;
  int z_cal_hi = z_cal_low + cal_z;
  for (y = y_cal_low; y < y_cal_hi; y++) {
    for (z = z_cal_low; z < z_cal_hi; z++) {
      if (cal_coverage == ELLIPTICAL) {
        float l_y = (y + .5 - ky_center) / (cal_y / 2);
        float l_z = (z + .5 - kz_center) / (cal_z / 2);
        float l_r = sqrt(l_y * l_y + l_z * l_z);
        if (l_r > 1.0) {
          continue;
        }
      }
      float y2 = (y + .5 - ky_center) / ky_radius;
      float z2 = (z + .5 - kz_center) / kz_radius;

      KS_KCOORD coord = {y, z, (float)sqrt((y2 * y2) + (z2 * z2)), (float)atan2(z2, y2)};
      queue[queue_size++] = coord;
      K[num_coords++] = coord;
      set_grid(grid, Nz, coord.y, coord.z);
    }
  }


  while (queue_size > 0) {
    /* Pick a random element in active list */
    int active_idx = rand() % queue_size; 
    KS_KCOORD active_coord = queue[active_idx];

    float spacing = ks_poisson_disc_min_spacing(active_coord.y, active_coord.z, ky_center, kz_center, min_spacing, max_spacing, pattern);

    int success = 0;
    int attempt;
    for (attempt = 0; attempt < num_attempts; attempt++) {
      /* Spawn a candidate in the circular annulus around the active coordinate */
      float spawn_angle = rand_float_in_range(-3.14159265, 3.14159265);
      float spawn_distance = rand_float_in_range(spacing, 2*spacing);

      float cand_y = active_coord.y + 0.5 + spawn_distance * sin(spawn_angle);
      float cand_z = active_coord.z + 0.5 + spawn_distance * cos(spawn_angle);

      /* Snap the candidate to the grid */
      float y2 = ((int)cand_y + .5 - ky_center) / ky_radius;
      float z2 = ((int)cand_z + .5 - kz_center) / kz_radius;
      KS_KCOORD cand = {(int)(cand_y), (int)(cand_z), (float)sqrt((y2 * y2) + (z2 * z2)), (float)atan2(z2, y2) };
      if (cand.y < 0 || cand.y > (Ny-1) ||
          cand.z < 0 || cand.z > (Nz-1)  ) {
            continue;
      }

      /* Check if candidate is within active radius (it might have snapped closer than allowed) */
      if (sqrt(pow(active_coord.y - cand.y,2) + pow(active_coord.z - cand.z,2)) < spacing) {
        continue;
      }
      /* Retrieve the radius at the candidate position */
      float radius_candidate = ks_poisson_disc_min_spacing(cand.y, cand.z, ky_center, kz_center, min_spacing, max_spacing, pattern);
      int conflict = check_conflicts(grid, Ny, Nz, cand, radius_candidate);
      if (conflict != 1) {
        set_grid(grid, Nz, cand.y, cand.z);
        K[num_coords++] = cand;
        queue[queue_size++] = cand;
        success++;
        break;
      }
    }

    if (success == 0) {
      remove_element(queue, active_idx, queue_size--);
    }

  }

  free(queue);
  free(grid);
  K = (KS_KCOORD*)realloc(kacq->coords, num_coords* sizeof(KS_KCOORD)); /* Shrink */
  if (!K) {
    return KS_THROW("Failed reallocation");
  }
  kacq->coords = K;
  kacq->num_coords = num_coords;

  for (idx = 0; idx < kacq->num_coords; idx++) {
    kacq->coords[idx].y -= Ny/2;
    kacq->coords[idx].z -= Nz/2;
  }

  kacq->matrix_size[YGRAD] = Ny;
  kacq->matrix_size[ZGRAD] = Nz > 1 ? Nz : KS_NOTSET;

  return SUCCESS;
}




STATUS ks_generate_3d_coords_epi(KS_KSPACE_ACQ* kacq,
                                 const KS_EPI *epitrain,
                                 const ks_enum_epiblipsign blipsign,
                                 const int Ry,
                                 const int caipi_delta,
                                 const KS_PF_EARLYLATE pf_earlylate) {

  /* Interleaves refers to the number of different shifts along ky */
  const int max_numileaves = epitrain->blipphaser.R;
  const int acc_numileaves = epitrain->blipphaser.R / Ry;

  if (epitrain->blipphaser.R < 1 || Ry < 1) {
    return KS_THROW("Acceleration factors must be positive");
  }

  if (acc_numileaves < 1) {
    return KS_THROW("The requested acceleration (%d) is greater than the maximum allowed (%d) by the EPI train", Ry, epitrain->blipphaser.R);
  }

  if (epitrain->blipphaser.R % Ry) {
    return KS_THROW("The maximum allowed acceleration by the EPI train must be divisible by the requested one");
  }

  /* Overallocate for now */
  const int Nz = epitrain->zphaser.res > 0 ? epitrain->zphaser.numlinestoacq : 1;
  KS_KCOORD* K = (KS_KCOORD*)realloc(kacq->coords, Nz*max_numileaves * sizeof(KS_KCOORD));
  if (!K) {
    return KS_THROW("Failed (re)allocation");
  }
  kacq->coords = K;
  kacq->num_coords = 0;

  /* First compute the starting ky line if the top part were to be sampled */
  const int bottom_line = max_numileaves * epitrain->etl - 1;
  int base_kyview = pf_earlylate == KS_PF_LATE ?
    0 : /* half of kspace from the center */
    bottom_line; /* might be less than half of kspace from the center if partial Forurier is active */

  /* flip the starting ky coordinate if the combination of blip sign and early/late eccho choices means that
     the bottom part of the kspace needs to be sampled. */
  if ((blipsign == KS_EPI_NEGBLIPS && pf_earlylate == KS_PF_EARLY) ||
      (blipsign == KS_EPI_POSBLIPS && pf_earlylate == KS_PF_LATE)) {
    base_kyview = epitrain->blipphaser.res - 1 - base_kyview;
  }
  
  const double ky_center = (epitrain->blipphaser.res-1)/2.0;
  const double kz_center = epitrain->zphaser.res > 0 ? (epitrain->zphaser.res-1)/2.0 : -1;

  int kzidx = 0, kyidx;
  int kzview = KS_NOTSET;
  int is_cal = 0;
  int shot = 0;
  do {
    if (epitrain->zphaser.res != KS_NOTSET) {

      kzview = epitrain->zphaser.linetoacq[kzidx];

      if (epitrain->zphaser.R > 1) {
        if (kzidx < epitrain->zphaser.numlinestoacq-1) {
          is_cal =
            epitrain->zphaser.nacslines > 0 &&
            abs(epitrain->zphaser.linetoacq[kzidx+1]- kzview) == 1;
        }
      } else {
        is_cal = abs(epitrain->zphaser.res/2.0 - kzview)*2.0 < epitrain->zphaser.nacslines;
      }
    }

    const int numileaves = is_cal ? max_numileaves : acc_numileaves;
    const int step = is_cal ? 1 : Ry;

    for (kyidx = 0; kyidx < numileaves; ++kyidx) {
      int shift = kyidx * step;

      if (epitrain->zphaser.res != KS_NOTSET) {
        /* CAIPIRINHA shift */
        shift = (shift + (kzview/epitrain->zphaser.R) * caipi_delta) % max_numileaves;
      }

      /* Positive blips go from bottom up, that is from high kyviews to low kyviews */
      shift *= -blipsign;
      
      const int kyview = base_kyview + shift;
      /* Calculate the symmetric coordinates normalized to [-1,1].
         Note that, if kzview is -1, so is kz_center. This results in a z coordinate of zero. */
      const double y = (kyview - ky_center)/ky_center;
      const double z = (kzview - kz_center)/kz_center; 
      KS_KCOORD c = {kyview - epitrain->blipphaser.res/2,
                     kzview - epitrain->zphaser.res/2,
                     (float)sqrt((y * y) + (z * z)),
                     (float)atan2(z, y),
                     };

      K[shot] = c;
      ++shot;
    }

    ++kzidx;

  } while(epitrain->zphaser.res != KS_NOTSET && kzidx < epitrain->zphaser.numlinestoacq);

  /* Downsize if necessary */
  K = (KS_KCOORD*)realloc(K, shot * sizeof(KS_KCOORD));
  if (!K) {
    return KS_THROW("Failed (re)allocation");
  }

  kacq->coords = K;
  kacq->num_coords = shot;
  kacq->matrix_size[YGRAD] = epitrain->blipphaser.res;
  kacq->matrix_size[ZGRAD] = epitrain->zphaser.res > 0 ? epitrain->zphaser.numlinestoacq : KS_NOTSET;

  return SUCCESS;
}



