#ifndef CATA_SCORES_UI_HPP
#define CATA_SCORES_UI_HPP

class achievements_tracker;
class kill_tracker;
class stats_tracker;

void show_scores_ui( const achievements_tracker &achievements, stats_tracker &,
                     const kill_tracker & );

#endif // CATA_SCORES_UI_HPP
