#pragma once
#ifndef CATA_SRC_SCORES_UI_H
#define CATA_SRC_SCORES_UI_H

#if defined(IMGUI)
void show_scores_ui();
#else
class achievements_tracker;
class kill_tracker;
class stats_tracker;

void show_scores_ui( const achievements_tracker &achievements, stats_tracker &,
                     const kill_tracker & );
#endif
#endif // CATA_SRC_SCORES_UI_H
