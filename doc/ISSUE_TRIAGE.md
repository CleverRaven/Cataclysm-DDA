## Why triage?

Triage is a process of sanity checking and prioritizing issues. The term is adopted from medical contexts where patients are briefly examined to decide whether they need immediate care, or if their issue can wait until patients at greater risk can be handled first.
Software issue triage is not life-or-death, but it does adress the same fundamental problem, which is that there is a limited amount of attention and skill that can be brought to bear on issues at any given time, and some issues have higher *impact* than others.

If issues are properly triaged, contributors with time available can focus on getting things done instead of searching through issues for something to work on and fix.

## How does triage work?
Look at new issues or revisit old issues after a while and apply various labels:
1. Characterize as a bug report or a feature suggestion
2. if bug report:
    1. label with priority
    2. check for validity
3. if feature suggestion:
    1. evaluate for validity
4. in either case, also apply labels indicating what areas of the game it impacts, i.e. content/json vs features/code, items vs monsters vs translation etc.

### Is this a bug report or a feature request?
Sometimes this is simple, is the game crashing, is it losing save data? If so it's definitely a bug.
Likewise, new content or new ways to interact with the game are always feature requests.
Other times this is a surprisingly tricky question. "The game isn't working like I expect it to" doesn't actually mean the game is doing anything wrong, and even if it is (i.e. it sets up an expectation but doesn't meet it), the problem might be where the expectation is set rather than the point where the expectation is (not) met.

At the end of the day, this is a judgement call about what the triager understands the game to be designed to do vs what it's actually doing. If the game's behavior is consistent with the design of the game, we'd call that a request to change it a feature request, if the game is not acting as designed, then it's a bug.

### Bug priority
Roughly our priority ordering is:
1. crashes
2. loss of data. This includes save migration from supported stable versions, but not ancient stable versions or experimental versions. To be clear migration *to* stable versions from experimental versions *is* included. This also includes chunks of map being lost or overwritten.
3. character death or item loss due to bugs (this is unfortunately not itself a bug but the outcome of a bug co it can be very subjective)
4. blatant "lack of consistency" bugs (characters, items or monsters having stats an order of magnitude or so off from reality, mechanics that make no sense).
5. poor UX due to e.g. excessive keypresses for an activity, tricky or misleading input, poor wording of UI elements. (maybe actually a feature request)
6. minor inconsistencies e.g. things that are only apparent by comparing json values, typos, and so on.

This is a bit more granular than necessary, 1, 2 and 3 are critical, 4 and 5 are important and everything else is null priority.

The reason for priority triage is in part to surface the highest priority issues for more attention, and partially to guide how much effort we spend on tracking the bug.
- If an issue says the game is crashing or losing data, just mark it as crashing/data loss/high priority, and that bug needs to stick around until we have expended some meaningful effort on demonstrating that issue doesn't happen, either because there is some error with the report or because it's been fixed.
- In the middle are a bunch of subjective "the game didn't do what I expect" issues, the process for them is they require confirmation, if they are confirmed they will stick around, and if not the stale action will prune them. You can confirm issues by reproducing locally as part of triage, but it can be very time consuming so it isn't required.
- On the other extreme, if a bug report boils down to "I think this number should be a 5 instead of a 6", we can't afford for that to take up a slot in the issue tracker, and it just needs to be closed.

### Bug validity
Bugs can be reported based on some kind of misunderstanding of how the game works and sometimes even not exist at all. It's terribly easy to blame some unrelated feature or change for behavior someone doesn't like.
Ideally all bug reports come with a clear statement of how to reproduce the behavior being reported and a clear statement of what should have happened instead. Unfortunately many bug reports don't have these basic elements, and the triage outcome needs to be to reply in the issue that these elements are missing.
You CAN add those details yourself if you are able to and so inclined, but it's not a necessary part of the process.
If an issue does not have these elements, and especially if no one else is able to reproduce the issue other than the reporter, that issue is unfortunately just noise, and the eventual outcome is closing the issue until someone can provide those details.

### Feature request validity
Feature requests should be specific, achievable and relevant (some of you might be thinking "SMART goals" and you'd be right, but Measurable and Time-bound aren't applicable).  
- Specific means the issue should clearly describe what the feature should do or what content should be added.  
- Achievable both means the feature should be not just *possible*, but also *feasible*. "Use generative AI for NPC conversations" fails both. "Use detailed nutrition information to make food better" is possible but would require a prohibitive amount of work.  
- Relevant means that the feature/content should *improve the game*, there's a certain amount of "add more stuff" that's self-justifying under the vague rationale of "make the world more believable", but taken to an extreme this is an unsustainable thing to do. Likewise features for the sake of features that don't actually impact the game in any way should be discouraged, or at least scoped to match the effort required to implement them with the benefit they provide.

#### Outcomes for invalid feature requests
Feature requests don't get triaged for priority the way bugs do, but what can happen is a feature request is "too small to track", i.e. it's more effort to open an issue and fix it than to just make a PR, so just make a PR.
i.e. if you want to fix typos, don't make an issue reporting the typos, just crack open the file in question and make the typo fix.
Likewise if you want a specific new item in the game, or change the stats of a specific item slightly, either do it in a PR or cheerlead for it elsewhere.
