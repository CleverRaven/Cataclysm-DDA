// hunger thresholds and messages
const int HUNGER_FREQUENCY = 20;
const int HUNGER_MINOR = 3000;
const int HUNGER_MAJOR = 4000;
const int HUNGER_CRITICAL = 5000;
const int HUNGER_DEATH = 6000;

const char* HUNGER_MINOR_MSG = _("Your stomach feels so empty...");
const char* HUNGER_MAJOR_MSG = _("You are STARVING!");
const char* HUNGER_CRITICAL_MSG = _("Food...");
const char* HUNGER_DEATH_MSG = _("You have starved to death.");
const char* HUNGER_MALE_MEMORIAL_MSG = pgettext("memorial_male", "Died of starvation");
const char* HUNGER_FEMALE_MEMORIAL_MSG = pgettext("memorial_female", "Died of starvation");

// thirst thresholds and messages
const int THIRST_FREQUENCY = 20;
const int THIRST_MINOR = 600;
const int THIRST_MAJOR = 800;
const int THIRST_CRITICAL = 1000;
const int THIRST_DEATH = 1200;

const char* THIRST_MINOR_MSG = _("Your mouth feels so dry...");
const char* THIRST_MAJOR_MSG = _("You are THIRSTY!");
const char* THIRST_CRITICAL_MSG = _("Even your eyes feel dry...");
const char* THIRST_DEATH_MSG = _("You have died of dehydration.");
const char* THIRST_MALE_MEMORIAL_MSG = pgettext("memorial_male", "Died of thirst.");
const char* THIRST_FEMALE_MEMORIAL_MSG = pgettext("memorial_female", "Died of thirst.");

// fatigue thresholds and messages
const int FATIGUE_FREQUENCY = 50;
const int FATIGUE_TRIVIAL = 383;
const int FATIGUE_MINOR = 575;
const int FATIGUE_MAJOR = 600;
const int FATIGUE_CRITICAL = 700;
const int FATIGUE_DIRE = 800;
const int FATIGUE_SLEEP = 1000;

const char* FATIGUE_TRIVIAL_MSG = _("*yawn* You should really get some sleep.");
const char* FATIGUE_MINOR_MSG = _("How much longer until bedtime?");
const char* FATIGUE_MAJOR_MSG = _("You're too tired to stop yawning.");
const char* FATIGUE_CRITICAL_MSG = _("You feel like you haven't slept in days.");
const char* FATIGUE_DIRE_MSG = _("Anywhere would be a good place to sleep...");
const char* FATIGUE_SLEEP_MSG = _("Survivor sleep now.");
const char* FATIGUE_MALE_MEMORIAL_MSG = pgettext("memorial_male", "Succumbed to lack of sleep");
const char* FATIGUE_FEMALE_MEMORIAL_MSG = pgettext("memorial_female", "Succumbed to lack of sleep");
