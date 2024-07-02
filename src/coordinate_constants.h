#pragma once
#ifndef CATA_SRC_COORDINATE_CONSTANTS_H
#define CATA_SRC_COORDINATE_CONSTANTS_H

#include "coords_fwd.h"
#include "point.h"

// Typed versions of the standard set of relative location constants.
const point_rel_ms point_rel_ms_min{ point_min };
const point_rel_ms point_rel_ms_zero { point_zero };
const point_rel_ms point_rel_ms_north{point_north};
const point_rel_ms point_rel_ms_north_east{ point_north_east };
const point_rel_ms point_rel_ms_east{point_east};
const point_rel_ms point_rel_ms_south_east{ point_south_east };
const point_rel_ms point_rel_ms_south {point_south};
const point_rel_ms point_rel_ms_south_west{ point_south_west };
const point_rel_ms point_rel_ms_west {point_west};
const point_rel_ms point_rel_ms_north_west{ point_north_west };

const point_abs_ms point_abs_ms_min{ point_min };
const point_sm_ms point_sm_ms_min{ point_min };
const point_omt_ms point_omt_ms_min{ point_min };
const point_bub_ms point_bub_ms_min{ point_min };

const point_abs_ms point_abs_ms_zero{ point_zero };
const point_sm_ms point_sm_ms_zero{ point_zero };
const point_omt_ms point_omt_ms_zero{ point_zero };
const point_bub_ms point_bub_ms_zero{ point_zero };

const point_rel_sm point_rel_sm_min{ point_min };
const point_rel_sm point_rel_sm_zero{ point_zero };
const point_rel_sm point_rel_sm_north{ point_north };
const point_rel_sm point_rel_sm_north_east{ point_north_east };
const point_rel_sm point_rel_sm_east{ point_east };
const point_rel_sm point_rel_sm_south_east{ point_south_east };
const point_rel_sm point_rel_sm_south{ point_south };
const point_rel_sm point_rel_sm_south_west{ point_south_west };
const point_rel_sm point_rel_sm_west{ point_west };
const point_rel_sm point_rel_sm_north_west{ point_north_west };

const point_abs_sm point_abs_sm_min{ point_min };
const point_omt_sm point_sm_sm_min{ point_min };
const point_om_sm point_omt_sm_min{ point_min };
const point_bub_sm point_bub_sm_min{ point_min };

const point_abs_sm point_abs_sm_zero{ point_zero };
const point_omt_sm point_sm_sm_zero{ point_zero };
const point_om_sm point_omt_sm_zero{ point_zero };
const point_bub_sm point_bub_sm_zero{ point_zero };

const point_rel_omt point_rel_omt_min{ point_min };
const point_rel_omt point_rel_omt_zero{ point_zero };
const point_rel_omt point_rel_omt_north{ point_north };
const point_rel_omt point_rel_omt_north_east{ point_north_east };
const point_rel_omt point_rel_omt_east{ point_east };
const point_rel_omt point_rel_omt_south_east{ point_south_east };
const point_rel_omt point_rel_omt_south{ point_south };
const point_rel_omt point_rel_omt_south_west{ point_south_west };
const point_rel_omt point_rel_omt_west{ point_west };
const point_rel_omt point_rel_omt_north_west{ point_north_west };

const point_rel_om point_rel_om_min{ point_min };
const point_rel_om point_rel_om_zero{ point_zero };
const point_rel_om point_rel_om_north{ point_north };
const point_rel_om point_rel_om_north_east{ point_north_east };
const point_rel_om point_rel_om_east{ point_east };
const point_rel_om point_rel_om_south_east{ point_south_east };
const point_rel_om point_rel_om_south{ point_south };
const point_rel_om point_rel_om_south_west{ point_south_west };
const point_rel_om point_rel_om_west{ point_west };
const point_rel_om point_rel_om_north_west{ point_north_west };

const point_abs_omt point_abs_omt_min{ point_min };
const point_om_omt point_om_omt_min{ point_min };
const point_abs_seg point_abs_seg_min{ point_min };
const point_abs_om point_abs_om_min{ point_min };

const point_abs_omt point_abs_omt_zero{ point_zero };
const point_om_omt point_om_omt_zero{ point_zero };
const point_abs_seg point_abs_seg_zero{ point_zero };
const point_abs_om point_abs_om_zero{ point_zero };

const tripoint_rel_ms tripoint_rel_ms_min{ tripoint_min };
const tripoint_rel_ms tripoint_rel_ms_zero{ tripoint_zero };
const tripoint_rel_ms tripoint_rel_ms_north{ tripoint_north };
const tripoint_rel_ms tripoint_rel_ms_north_east{ tripoint_north_east };
const tripoint_rel_ms tripoint_rel_ms_east{ tripoint_east };
const tripoint_rel_ms tripoint_rel_ms_south_east{ tripoint_south_east };
const tripoint_rel_ms tripoint_rel_ms_south{ tripoint_south };
const tripoint_rel_ms tripoint_rel_ms_south_west{ tripoint_south_west };
const tripoint_rel_ms tripoint_rel_ms_west{ tripoint_west };
const tripoint_rel_ms tripoint_rel_ms_north_west{ tripoint_north_west };
const tripoint_rel_ms tripoint_rel_ms_above{ tripoint_above };
const tripoint_rel_ms tripoint_rel_ms_below{ tripoint_below };

const tripoint_abs_ms tripoint_abs_ms_min{ tripoint_min };
const tripoint_sm_ms tripoint_sm_ms_min{ tripoint_min };
const tripoint_omt_ms tripoint_omt_ms_min{ tripoint_min };
const tripoint_bub_ms tripoint_bub_ms_min{ tripoint_min };

const tripoint_abs_ms tripoint_abs_ms_zero{ tripoint_zero };
const tripoint_sm_ms tripoint_sm_ms_zero{ tripoint_zero };
const tripoint_omt_ms tripoint_omt_ms_zero{ tripoint_zero };
const tripoint_bub_ms tripoint_bub_ms_zero{ tripoint_zero };

const tripoint_rel_sm tripoint_rel_sm_min{ tripoint_min };
const tripoint_rel_sm tripoint_rel_sm_zero{ tripoint_zero };
const tripoint_rel_sm tripoint_rel_sm_north{ tripoint_north };
const tripoint_rel_sm tripoint_rel_sm_north_east{ tripoint_north_east };
const tripoint_rel_sm tripoint_rel_sm_east{ tripoint_east };
const tripoint_rel_sm tripoint_rel_sm_south_east{ tripoint_south_east };
const tripoint_rel_sm tripoint_rel_sm_south{ tripoint_south };
const tripoint_rel_sm tripoint_rel_sm_south_west{ tripoint_south_west };
const tripoint_rel_sm tripoint_rel_sm_west{ tripoint_west };
const tripoint_rel_sm tripoint_rel_sm_north_west{ tripoint_north_west };
const tripoint_rel_sm tripoint_rel_sm_above{ tripoint_above };
const tripoint_rel_sm tripoint_rel_sm_below{ tripoint_below };

const tripoint_abs_sm tripoint_abs_sm_min{ tripoint_min };
const tripoint_om_sm tripoint_om_sm_min{ tripoint_min };
const tripoint_bub_sm tripoint_bub_sm_min{ tripoint_min };

const tripoint_abs_sm tripoint_abs_sm_zero{ tripoint_zero };
const tripoint_om_sm tripoint_om_sm_zero{ tripoint_zero };
const tripoint_bub_sm tripoint_bub_sm_zero{ tripoint_zero };

const tripoint_rel_omt tripoint_rel_omt_min{ tripoint_min };
const tripoint_rel_omt tripoint_rel_omt_zero{ tripoint_zero };
const tripoint_rel_omt tripoint_rel_omt_north{ tripoint_north };
const tripoint_rel_omt tripoint_rel_omt_north_east{ tripoint_north_east };
const tripoint_rel_omt tripoint_rel_omt_east{ tripoint_east };
const tripoint_rel_omt tripoint_rel_omt_south_east{ tripoint_south_east };
const tripoint_rel_omt tripoint_rel_omt_south{ tripoint_south };
const tripoint_rel_omt tripoint_rel_omt_south_west{ tripoint_south_west };
const tripoint_rel_omt tripoint_rel_omt_west{ tripoint_west };
const tripoint_rel_omt tripoint_rel_omt_north_west{ tripoint_north_west };
const tripoint_rel_omt tripoint_rel_omt_above{ tripoint_above };
const tripoint_rel_omt tripoint_rel_omt_below{ tripoint_below };

const tripoint_abs_omt tripoint_abs_omt_min{ tripoint_min };
const tripoint_om_omt tripoint_om_omt_min{ tripoint_min };
const tripoint_abs_seg tripoint_abs_seg_min{ tripoint_min };
const tripoint_abs_om tripoint_abs_om_min{ tripoint_min };

const tripoint_abs_omt tripoint_abs_omt_zero{ tripoint_zero };
const tripoint_om_omt tripoint_om_omt_zero{ tripoint_zero };
const tripoint_abs_seg tripoint_abs_seg_zero{ tripoint_zero };
const tripoint_abs_om tripoint_abs_om_zero{ tripoint_zero };
#endif // CATA_SRC_COORDINATE_CONSTANTS_H
