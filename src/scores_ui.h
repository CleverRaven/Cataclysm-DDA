#ifndef CATA_SRC_SCORES_UI_H
#define CATA_SRC_SCORES_UI_H

class achievements_tracker;
class kill_tracker;
class stats_tracker;

void show_scores_ui( const achievements_tracker &achievements, stats_tracker &,
                     const kill_tracker & );

#endif // CATA_SRC_SCORES_UI_H
