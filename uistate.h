#ifndef _UISTATE_H_
#define _UISTATE_H_
struct uistatedata {
  int adv_inv_leftsort;
  int adv_inv_rightsort;
  int adv_inv_leftarea;
  int adv_inv_rightarea;
  int adv_inv_leftindex;
  int adv_inv_rightindex;
  int adv_inv_leftpage;
  int adv_inv_rightpage;
  point adv_inv_last_coords;
  int last_inv_start, last_inv_sel;
  uistatedata() {
      adv_inv_leftsort = 1;
      adv_inv_rightsort = 1;
      adv_inv_leftarea = 5;
      adv_inv_rightarea = 0;
      adv_inv_leftindex = 0;
      adv_inv_rightindex=0;
      adv_inv_leftpage=0;
      adv_inv_rightpage=0;
      adv_inv_last_coords.x=-999;
      adv_inv_last_coords.y=-999;
      last_inv_start = -2;
      last_inv_sel = -2;
  };
};
extern uistatedata uistate;
#endif
