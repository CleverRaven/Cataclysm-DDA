# The DDA Development Process

**A guide for new and prospective contributors, or just for people curious about how the devs understand the project to work.**

This guide is intended to describe how the Cataclysm: Dark Days Ahead project is managed by the development team, and hopefully in the process, clear up common rumours and misconceptions about how it all works.  In this document the term "we" is intended to mean "CleverRaven as an organization", and does not necessarily reflect the individual opinions of anyone in CleverRaven.

This document assumes you have a basic understanding how GitHub works.  Please see the [guide for new contributors](https://github.com/CleverRaven/Cataclysm-DDA/wiki/Guide-to-adding-new-content-to-CDDA-for-first-time-contributors) for more info there.  You might also find helpful information in the [Frequently Made Suggestions doc](FREQUENTLY_MADE_SUGGESTIONS.md).  [This guide from GitHub](https://opensource.guide/how-to-contribute/#anatomy-of-an-open-source-project) also gives a decent run-down of the structure of any open source project.

## Table of Contents

* [The basic structure](#The-basic-structure)
* * [Experimental/Stable](#experimentalstable)
* * * [Why this format?](#Why-this-format)
* * [Mainline, in-repo mods, third party mods, and forks](#mainline-in-repo-mods-third-party-mods-and-forks)
* * [Project roles](#Project-roles)
* [FAQ for prospective contributors](#FAQ-for-prospective-contributors)
* * [How can I make sure my contribution will be accepted?](#How-can-I-make-sure-my-contribution-will-be-accepted)
* * * [What if different people have different opinions of my idea?](#What-if-different-people-have-different-opinions-of-my-idea)
* * * [How do I deal with a ton of comments and suggestions to my idea?](#How-do-I-deal-with-a-ton-of-comments-and-suggestions-to-my-idea)
* * * [Do a lot of things get merged to the project, then reverted?](#Do-a-lot-of-things-get-merged-to-the-project-then-reverted)
* * * [Am I allowed to do what I want with an existing NPC/faction/feature?](#Am-I-allowed-to-do-what-I-want-with-an-existing-NPCfactionfeature)
* * [Is the Discord strict?](#Is-the-Discord-strict)
* * [How does player feedback affect the project's development?](#How-does-player-feedback-affect-the-projects-development)
* * * [Do we care about the players?](#Do-we-care-about-the-players)
* [Supplemental Material](#Supplemental-Material)
* * [A Very Brief History of DDA](#A-Very-Brief-History-of-DDA)
* * * [Why did Kevin do this to me?](#Why-did-Kevin-do-this-to-me)
* * [Realism as a design goal](#Realism-as-a-design-goal)
* * [Why did X system get worked on while the more important Y system didn't](#Why-did-X-system-get-worked-on-while-the-more-important-Y-system-didnt)
* * * [Why did X system get implemented with Y, but not Z](#Why-did-X-system-get-implemented-with-Y-but-not-Z)
* [The Bottom Line](#The-Bottom-Line)

## The basic concept 

At its core, CDDA is a survival simulation game.  [The design doc outlines what we mean by this](./Lore/design-doc.md).  The project is led by Kevin Granade, who owns CleverRaven and therefore this fork of the code.  As lead developer, Kevin's main job in the project is to be the *last word* if one is needed.  Most of the time, we don't need his final arbitration to know if something is going to fit or not.[^code]  The rest of the project's structure is organized chaos, and understanding it is daunting at first.

### Experimental/Stable

One of the most important concepts to understand in our project is the **experimental/stable branch** system.

The intention here is that if you want to *play* CDDA, you'll play on stable.  This is a curated snapshot of the game with as few major bugs and balance issues as possible.  Sometimes, features have to ship in states we consider unfinished, because of the volunteer nature of the game, but we try to keep stable balanced and fun.[^fun]

On the other hand, in the experimental branch, we allow things to get broken.  We often allow (trusted) contributors to mess something up, especially if the code base they're adding is a strong foundation to build on.  In a volunteer-driven project, sometimes these can be slow to fix.  That's why we don't really recommend that people play experimental as their main way to enjoy the game.  Of course, many people do, but from our perspective, those people are more like *playtesters*, not *players*.  It might help to understand how we process playtest feedback:
- First, we assess if the feedback is valid.  This is hard, and it becomes harder when news of a change is circulated to people that haven't yet tested the change.[^whytheory]
- Once we're sure the concern is actually happening, we check if it's intended behaviour or not.  If it's an outright bug, we try to fix it.  If it's an unintended consequence of complex systems interacting, often the solution is to think about how the mechanic should work in real life.[^bows]
- If the behaviour is intended, we consider why people are frustrated by it.  Is player-end feedback a problem?  Is the UI creating tedium?  This is an area where things can languish for a while, but **it doesn't mean we don't care**, it's just that these problems are hard to fix, and often take different skills than the original addition.  This is also where the experimental/stable thing becomes relevant.  For example, on a few occasions, we've patched the behaviour out and then put it back in after stable.  This is to preserve gameplay for the people playing the curated, stable version of the game, while also continuing to work on the features we want to include.

#### Why this format?
Or, **Why not finish things before putting them in a public experimental release, or revert them if they're broken?**

There are dozens of different good reasons.  Let's look at just one, because it's possibly the most important and also the most difficult to see unless you really examine development over the long term.

Allowing things to get broken in experimental allows us to have big, ambitious projects.  If we forced ourselves to keep experimental 'smooth' all the time, contributors wouldn't be free to take risks.  Most of the complex things that make CDDA unique would have been almost impossible to create.  We'd be less welcoming to less experienced programmers, even if we didn't want to be.  In the long term we wouldn't have a better player experience to show for it, because many of our most interesting changes come about as the product of long - often years long - projects that remain in a messy state as we sort them out.  This is the trick we've found that allows unpaid volunteers to develop amazing systems that would be at home in a professional game, while having only a few hours a week of their time to contribute.

### Mainline, in-repo mods, third party mods, and forks

Cataclysm: DDA is one version of the 'Cataclysm' source code.  There are many others.  Even your own branch on your own GH account is a fork, technically.  The main game is often called "mainline".

* **In-repo mods** are mods that we distribute with the game.  [We have a fairly specific set of criteria](https://docs.cataclysmdda.org/IN_REPO_MODS.html) for what mods we'll include.  If you have a mod you are comfortable releasing under CC-BY-SA and want to include it, and it meets these criteria, you'd probably be fine to include it in mainline and we'd probably love to have it.
* **Third party mods** are mods that aren't in the main repository, either because they don't meet the criteria, or because their owners just haven't decided to add them.  These mods aren't lesser, they're just not our responsibility.
* **Forks** are other versions of the Cataclysm source code, run by different people, presumably with different design goals.  We consider forks an important part of a healthy open-source community.  Having healthy forks lets everyone develop and play the way they want, ideally without pointless drama.  The most well-known fork is Bright Nights, which is owned by a former dev with whom we are still on friendly terms.

### Project roles

This is a quick summary of the different terms we use for different groups within the project.  Almost all of these terms are quite fuzzy and imprecise.

* **CleverRaven** is the github organization that owns this fork of CDDA.  That might sound precise, but it's actually a little hard to define who is or is not part of CleverRaven and whether or not they have any official authority.  Not all CleverRaven members are lead developers with merge permissions, but anyone who is a member of CleverRaven has some trust and experience with the project.
* The **Project Lead** is Kevin Granade.  He mainly serves as the final voice in what can go into the project, and arbitrates disputes between other members of the team.
* **Senior developers** have merge permissions with CleverRaven and have been around a long time.  This is a very fuzzy role, none of us know exactly what makes a person a "senior" developer versus any other kind of "developer".  It just kind of happens, usually because they're doing leadership things and not being told to stop.[^erk]  Some of the senior devs have gold names on Discord, but because we love confusion, most do not.
* **Developers** are members of CleverRaven with merge permissions, aka "mergers".  These folks are trusted enough to be allowed to merge to the main project branch.  They are the main workforce of the game's management team.  Developers usually have green or gold names on Discord.
* **Collaborators** are contributors that have been around a while, and shown that they "get" the direction of the project.  We mark their names blue on Discord to indicate this; the purpose of the role is to help newer users to identify if the feedback they're getting is from a fellow new person, or from someone with a bit more experience.  Collaborators don't speak for CleverRaven necessarily, but most of the time they know what they're talking about.
* **The dev team** is a vague term that means, roughly, "the developers and sometimes the collaborators too".
* **Contributors** are people who have added something to the game, anything.  Code, art, translations, writing, are all included.  While this is the most numerous group, we also think it's the most important one.  Everyone from Kevin onward is a contributor.  Contributors have purple names on Discord.
* **Core contributors** is another vague term that we use sometimes, meaning "the developers, the collaborators, and a bunch of the contributors that have been around a lot".
* **Players** are the people whose lives we try to ruin, who somehow keep coming back for more.  What is their deal?  We may never know.

## FAQ for prospective contributors

This is built from concerns we've heard from other new contributors to the project, things they wish they'd understood when starting out.  Here's the most important TL;DR points:

1.  Most simple, reasonable contributions get accepted.  For larger projects, you might want to get more experience with the project or clear it with a developer first.
2.  If it sounds like your small job is being turned into a big unmanageable job, check if the ideas being thrown around are things *you* have to do, or just people getting excited.  Usually it's just excitement, it means you have a cool idea but not that you need to do a bunch more stuff.
3.  If you're getting negative feedback or being told you have to do something a certain way, make sure it's someone with authority before you start feeling discouraged.
4.  Please don't take it personally if you receive a short, direct response on a PR or issue.  Tone doesn't convey well in text, but the intent is to be completely clear and matter-of-fact, not to be unkind.

### How can I make sure my contribution will be accepted?

If what you're doing is simple, chances are you don't need to worry.  If you're new to the project and have big ideas for what you want to add, though, we strongly recommend you first discuss these with someone more experienced in the project.  There are three good ways to do that: create an Issue post on GitHub, chat in [the developer discord](https://discord.gg/hqWgHC8TKY) in the Development or CDDA-Discussion channel, or post in [the discourse forum](https://discourse.cataclysmdda.org/)[^discourse] about your ideas.  The larger the project you plan to add, the more important it is to check in first.  That isn't to say big things can't get in without prior approval - it happens all the time - but it's always a bigger risk, especially if you're new.

Most PRs do get accepted.  If you are the sort of person who won't really mind if you do some work and find it's going to have to go back to the drawing board, you can skip this entire document.  You'll be fine.  If that possibility concerns you though, reading on may help you to navigate collaborative feedback in a large and mostly unorganized community.

If you've created an issue or a discourse post and aren't getting much feedback, try reaching out on Discord as a good second measure.  Be patient and give us time to spot you, but don't be afraid to be persistent.  Due to the high activity on the project, stuff often slips beneath people's radars.  That doesn't mean we're not interested in what you've got.  It's just a side effect of it being a volunteer project where all of us have limited time to devote.

#### What if different people have different opinions of my idea?

The ultimate authority is Kevin Granade, as project lead.  After him, the developers (green and gold names on discord) are pretty likely to know what's what.  After them, the collaborators and moderators (people with blue or magenta names on discord) usually have a very good concept of the project direction.  Anyone else can of course offer an opinion, and often have great information, but take it with a grain of salt, especially if it conflicts with the above.  We often get well-meaning people speaking with confidence about elements of the project they don't fully understand yet.

On GitHub it can be a little harder to tell who is who, and we don't really have a way around that.  If you see somebody merging PRs, closing issue posts, or adding labels to issues and PRs though, that's a person who probably has a higher level of trust in the project.  There's also a "member" badge on the upper right of github posts, like this:
![image](https://github.com/I-am-Erk/Cataclysm-DDA/assets/45136638/6106b80f-274e-43c7-a937-58f601f035e6)  
If a person has a "member" badge there, it usually means they speak with some authority.  However, we all have different areas of the project we know better than others, so if a member of CleverRaven says something like "I don't know much about armpit code, but maybe you want to do X", they're telling you not to take their word too seriously.

#### My idea was rejected, changed beyond recognition, or requires a lot more than I expected.

This can happen for a few reasons.  If you're not sure which applies to you, feel free to ask for clarification.

* Sometimes, your idea sparks a huge conversation, and the next thing you know it seems like you're on the hook to overhaul armpit fart mechanics before you can start.  *Please, feel free to mention if you feel this way*.  This usually means you've hit something cool that we're all excited about.  It often **doesn't mean we expect you to add** whatever we've started riffing on, and you're fine to just put in whatever small thing you originally wanted.  We're all just huge nerds, and we know that sometimes we get carried away.
* Sometimes, what you want to add seems simple, but the simple solution has already been rejected because of complex flaws that will cause us problems down the road.  If you're not able to do the harder work, you may have to find a different thing to work on.  We won't think less of you for that, and we're sorry when this happens.
* Sometimes, what you want just isn't compatible with the core game.  It might go fine in a mod, and it might even [be fine to include that mod in the main repository](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/IN_REPO_MODS.md) depending on what it is.

An issue we run into occasionally is that our responses can be short and straightforward, which can rub people the wrong way.  The intent is to be matter-of-fact and completely clear, not unkind.  We have found that if we try to be *too gentle*, we leave room for confusion: people continue working on a thing that will never be OK to add, and wind up wasting more of their effort, which is a really bad outcome and more unkind in the long run.  Remember that, first, text doesn't convey tone well; and second, we don't want to make it seem open to debate if it isn't.  However, we guarantee that nobody wants you to feel badly for having ideas and excitement about the game.[^ajoke]

#### Do a lot of things get merged to the project, then reverted?

This is a thing that happens sometimes; because it's a rare and significant event, it often gets a bit of publicity and seems like something that happens more.  At time of writing there have been roughly 1-2 reversions per month in the last year, which is around 0.2% of the PRs merged.  Most of these are reverted for infrastructure reasons, and added back as soon as possible.  It's very important to understand that on our end, **we consider reversions a failure of the dev team** to adequately review submissions.  They mean that we messed up and we're fixing it as best we can.

The best way to protect yourself from getting something reverted, or rejected before merge, is to (1) discuss your project, especially with developers, and make sure it's appropriate, (2) [get someone to review your PR before it's merged](reviewing_PR_guide.md)), and (3) keep your changes to individually small PRs where they can be properly reviewed and studied before being added.  However, remember that *by far* most PRs are not reverted nor rejected.

#### How do I deal with a ton of comments and suggestions to my idea?

If you're feeling overwhelmed by the amount of work being piled on you, first, we recommend making sure the suggestions are coming from someone with some authority on the matter.  When in doubt, these issues are easiest to sort out on Discord, where it's easier to tell who has some authority.  On GitHub we're doing our best to make sure that project members get a user badge, like this:
![image](https://github.com/I-am-Erk/Cataclysm-DDA/assets/45136638/6106b80f-274e-43c7-a937-58f601f035e6)  
A "member" badge means this person has permissions with CleverRaven and so has a better chance of being correct, but if they say they're not an authority in this area, that means you shouldn't overvalue them either.  As a last resort, consider pinging kevingranade or I-am-Erk on github to come in and clarify, although we'd very much prefer you to come ask in Discord.  It can also help to check if the suggested changes are in line with any of the official guides and documentation.

If the requested changes are coming from a trusted source, and it's more than you feel you can handle, it's all right to say as much.  We might not mean that we expect *you* to do whatever is being suggested.  Other times, the thing you're trying to do is just not going to be as easy as it looked.  In that case, we'd prefer you to do it if you can, but it's all right to decide that we're asking for too much.  These problems can *usually* be avoided with a well-discussed GitHub/Discourse post or Discord conversation.  If you're extremely averse to having this happen, try to be patient and persistent and make sure you've got the go-ahead from a developer before beginning, but we want to emphasize that *most of the time, this doesn't happen*.  The bigger your contribution is, the more important it becomes to get clearance.

#### Am I allowed to do what I want with an existing NPC/faction/feature?

When something gets added to the project, as soon as the PR goes out into the world, the traditional sense of "ownership" is relinquished and it becomes a collaborative creation effort.  The person putting in the most effort is a sort of director for that content, but a lot of the time they finish their plans and there's plenty of room for more work to go in.  If you'd like to make drastic changes, you should first get an idea whether or not someone is actively working on the thing you plan to change.  For most of the content in the game, this is *not* the case, and we'd love someone to step up.[^npcs]  Probably the best place to find out about this is in the narrative-dev channel on Discord, since the main concern is making sure your new ideas won't conflict with the long term plans being worked on.

As a general rule, in fact, we would prefer it if new contributors would feel welcome to alter and improve on the existing NPCs in game before adding a bunch of new ones!  We have too many unfinished NPCs and quest lines as it is.  Don't let this stop you from adding a brand new NPC, but definitely don't feel held back by not knowing who "owns" an existing bit of the game.

### Is the Discord strict?

We don't think so, but obviously we're biased.  First, make sure you're looking at [the developer discord](https://discord.gg/hqWgHC8TKY) and not the fan discord or the old one that was subjected to a nazi takeover (seriously).

If you're on the developer discord to talk about development, it's very difficult to get banned or kicked out, and you'll always get warnings first[^spam].  If you're worried about getting in trouble for your memes or fun chat times, we suggest just going to [the community discord](https://discord.gg/byxwnAU
), where the rules are much more lax.  It's still fine to chat on the developer discord, but we intentionally run it mainly for very focused discussion of the game.   It's also the place to go to see the devs talking like normal people instead of giving dry technical responses on GH.

If someone is telling you they got banned from "devcord" for nothing at all without warning, there's a story they aren't telling you.  This just isn't a thing that happens.  Our ban records are public.

### How does player feedback affect the project's development?

There's a common misunderstanding about the role of non-contributor players, or even non-dev contributors, in affecting the project direction, which is that we don't care about player feedback.  It's not entirely false, but it's definitely not true; it should not be a reason to decide not to contribute.[^feedback]  However, the question of what we think of player feedback on the project side is actually complex and worth examining in detail.

#### Do we care about the players?

The short answer is, *yes, of course*, but this doesn't mean what some people might think.

1.  We are not selling a product, so we don't care about mass appeal or how many people play the game.[^massappeal]
2.  We already know what game we want, and it's not for everyone.  Nothing we add ever has the goal of making a game we wouldn't like to play ourselves.  Any feedback that amounts to "change this aspect of the game that *you* like into something *I* prefer" is just a non-starter.  We know that sometimes what you prefer has more mass appeal (see number 1) and we're sure it's more fun *for you*.
3.  This game is one of the most moddable games out there, but that's an outcome of data-driven design, not a specific goal we have.  In the end, it comes down to number 2 plus a bunch of details about managing the code and play balance.  It's intended that if you want something to run differently, you can mod it out; we just don't want to be on the hook to maintain all the preference mods and switches.  We tried it and it sucked.
4.  Many times, player feedback concerns come down to the playability of the experimental branch of the game.  See [the section on experimental/stable](#experimentalstable).  We almost never revert changes to experimental that we've added in order to test them, nor are we likely to relegate those changes to a mod or an option, because this interferes with testing.  See 2 and 3.  However, nearly all the time, whatever is bothering players about current experimental is something that will shake out and become sorted in time.
5.  The project isn't a democracy.  However, the project also isn't a dictatorship: it's more like an open forum with a moderator.  If you have opinions about something, make your case for it.[^venera]  If what you want doesn't conflict with the game design, you can very often change our minds.

We believe that the reason this project has thrived for so long is that we follow a consistent vision for the game, while allowing side projects and flavour tweaks to be included in the game.  It's fine if you just don't like what we like.  There are a few ways to reconcile that: you could work on a mod in repo, either your own or someone else's.  You could develop and share a third-party mod that we haven't got any control over at all.  You could join a fork, and help them out.  You could fork the project and demonstrate your own vision for it.  However, if you've looked around and you like where we're going, we'd love to have you join us, or even just play and have fun with our game.

##### So wait, it sounds like you're just going to do whatever you want and you don't care what the players say.

If you took that away from the above, we'll probably never see eye-to-eye.  However, we do actually care what players say.  Very often, constructive discussion with players results in our *goals* remaining the same, with new ideas on *how to reach those goals*.  Sometimes, we will listen to many well-reasoned arguments, and still not change our minds.  That is because 'listening' and 'caring about what you say' are not the same as 'doing what we're told'.  We've learned over the years that for some, this is never going to be sufficient, and again, we encourage those people to find ways to see their own goals achieved rather than continuing to be upset at us for not changing our own.

## Supplemental Material

This section isn't necessary to read, but contains (we hope) some useful fact-checking and some fun or interesting history.

### A Very Brief History of DDA

CDDA is a fork of a game called Cataclysm, developed by Whales.  Cataclysm was a grab-bag apocalypse roguelike set in a vague near-future setting.  Dark Days Ahead was forked by a group of people (Kevin Granade, TheDarklingWolf, and GlyphGryph) to be a more gritty, realistic zombie apocalypse game made from the same general concept.

We firmly believe that the reason CDDA has persevered and improved steadily since 2013 is that it has a consistent project direction.  We're also aware that many people claim the project direction changed at some point in the past.  [This interview from 2013](https://web.archive.org/web/20140211144953/http://www.jacehallshow.com/news/gaming/preview/20130626/ascii-goodness-living-breathing-3d-world-cataclysm-dark-days/) and [this one from about the same time](http://www.roguelikeradio.com/2013/07/episode-75-cataclysm-dark-days-ahead.html?m=1) show that the original developer team had much the same goals as we currently lay out in the design doc.  A lot of the misconception comes down to changes that were left to ride from the original Cataclysm before the DDA fork, and were changed around 2018-2020.

To clear up a few common rumours that circulate in certain channels:
- Kevin has always been in charge of the repo in his current capacity, even in the original oligarchy days, and there was never any sort of takeover.  The other original devs just left, for various reasons.  There was no falling out or hard feelings.
- Over the years we've lost a lot of developers.  The reason is not as dramatic as you might think: by and large, people get bored and move on to other places.  The number of major fallings-out can be counted on one hand, in over a decade of development.
- We didn't start a developer discord because of some secret community drama.  We just needed a place we could moderate for a focused discussion.
- That's also the same reason we started a developer subreddit.  It wasn't closed because of any internal strife, it was closed because we all used apps to moderate it, and Reddit shut down 3rd party apps.
- We don't have any animosity to the other major forks of the game.  Mostly, we are glad they exist, even though we know how some of them talk about us.  It's our opinion that it's a *good thing* if someone who doesn't like our way of managing the project has a different place to go.  We don't want to be dictators of our tiny corner of the internet, we want to make a neat game with as little drama as possible.

#### Why did Kevin do this to me?

This merits its own section because it's such a frequent by-line in various Cataclysm communities.  Kevin is mostly hands-off, although he is quite present and involved in a lot of discussion, but for some reason he is attributed as the primary source of all changes in Cataclysm (usually unpopular ones).  We know this is usually meant as a joke, but it obscures credit from our broad base of contributors.  In particular, it's helpful to understand that neither the devs, nor Kevin, have any "additive" influence.  Unless we put something in ourselves, the only way to get it added is to ask nicely.  At any given time there are around 50-100 different contributors of various levels working on the project, including both regulars and new faces.  If something was changed, it's most likely one of these people changed it, and Kevin simply didn't say 'no'.

As a classic example, when we first added the mod inclusion criteria and set a bunch of mods to obsolete, Kevin had very little involvement in the process.  It was a collaboration by around two dozen devs and contributors who were collectively tired of bugfixing mods none of us used.  Years down the road, we continue to see this cited as an example of Kevin exerting authority on the project, when his main involvement was to agree it was our call to make as the ones doing most of the work.[^controversy]  Similar situations have happened dozens of times.

### Realism as a design goal

Any prospective contributor should understand that we do not consider realism to be the goal of the design.  Rather, we are aiming for *verisimilitude*.  That is to say, most of the time, things should work the way you would expect them to work.[^movies]  Many things that would be more realistic may be nixed because of problems with play experience or play balance.  You can find a lot more detail about this in [the design document](./Lore/design-balance.md).

In general, the most common citation of "realism" online comes when glitches in experimental are mistaken for intended design.  An example was when `charges` were being removed as a part of a major *code infrastructure change* with no intended player impact at all.  This caused several months of glitches, like players having to move salt in individual pinches.  This was frequently called a "realism change", not a bug, in certain circles.  While funny, this attitude can be actively harmful to the project, deterring people from fixing bugs because they get the impression it's intended play.  If someone has said that a seemingly hostile and illogical game mechanic is the way it is due to "realism", they're probably wrong, either because the mechanic is bugged or because QoL improvements are desired to make it less frustrating.

### Why did X system get worked on while the more important Y system didn't?

Short answer: Because someone chose to work on X, not Y.

Longer answer: Sometimes, the more important system is much harder or more intimidating (eg NPC AI).  Most of the time, though, it's just that we have infinite things to work on and we have to pick something.  If a particular system is important and interesting to you, the only way to *ensure* someone works on it is to work on it yourself.  Chances are this is not as difficult as it sounds.  If you just can't do that, sometimes you can get headway by becoming a cheerleader for it.  Learn what needs to be done, and make it sound appealing to people who might know how to do it.  Generally, this isn't going to work well unless you also contribute your own stuff, but for example a dedicated tileset artist or translator might have a lot of sway in convincing a coder to do them a favour.

#### Why did X system get implemented with Y, but not Z?

For example, why did science equipment get added but you could only loot it in one location?

Usually, the person implementing it didn't want their PR to explode into a whole bunch of extra features (this doc even has a [FAQ entry on that problem](#How-do-I-deal-with-a-ton-of-comments-and-suggestions-to-my-idea)). This is a consequence of the game being experimental, not the intended design, and the intended fix is that someone should just add missing feature Z: eg, add science equipment to more spawn lists.  It's not the result of malice, it's the side effect of wanting to do something right, but not having the resources to be in every place at every time.

## The Bottom Line

Did you actually read all that, or just scroll down?  Massive kudos if you read it.

In the end, we hope you'll share our passion for this project and decide to come join us in contributing to it, and we hope that you find this document helpful in clearing up various odd misconceptions about how development works that have gathered over all the years of project management.

---
**Footnotes**

[^code]: Kevin is also much more likely to have strong opinions about code maintainability and architecture than he is about lore stuff, as a general rule.
[^fun]: At least by certain definitions of fun, which may or may not involve being eaten by a mi-go.
[^whytheory]: There's a tendency for theory crafting to be louder than playtests, and we've had severe lost work hours when devs have tried to solve a problem that turned out to not actually be happening in game.
[^bows]: A good example of this is that we tried to sort out bow balance for many years back and forth until finally adding weak points, because ultimately the issue was that arrows shouldn't be able to get through a lot of things bullets can, but almost all armours and defenses have gaps an archer can target.
[^erk]: This happened to I-am-Erk somehow and he's still not sure why he hasn't run away screaming.
[^discourse]: Discourse is a less active forum, but that can actually make it a better place to make a proposal.  Many of the devs still regularly check it, and you're much less likely to get missed in the background noise.  You just might have to be a little more patient as we're not checking it every day.
[^ajoke]: Unless your ideas are dumb, then…… but jokes aside, even a lot of the ideas we have to reject are *good ideas*, they're just either not practical for our project or they take it in directions we'd rather avoid.  We might be blunt about saying no, but we do seriously respect the creativity.
[^spam]: Unless you're a spambot, but then how are you reading this?
[^npcs]: At time of writing, the main faction to check in on is Hub01, which is being very actively developed by Candlebury with long term plans.  The Exodii are a distant second: there are a lot of plans here, but I-am-Erk is open about not having the time needed to actively develop them and would be very open to new directions if it meant getting them more actively handled.  This is as of May 2024 and subject to change.
[^feedback]: Also, if *you* want to listen to player feedback on your additions to the game, you're welcome to do it however you like!
[^massappeal]: In fact, in some ways, the more popular DDA gets the more trouble it causes from a project management standpoint: we get more drama and have to handle more top-heavy admin work than we would with a smaller project.  "Growing our audience" is something that has happened organically, and we love new people trying out the game, but at the same time it's definitely not a *goal*.
[^venera]: The general design is quite flexible.  For example when Venera3 started contributing, we were considering removing giant insects from the lore entirely.  Now we all have to deal with those infuriating giant wasps and it's all his fault.
[^controversy]: At time of writing this is still the most disliked PR in the entire project, and it's kind of interesting to note how little Kevin had to do with it.
[^movies]: To make matters worse, there are a lot of ways we expect things to work that turn out to not be really as clear as they seem, either due to popular "movie logic" misunderstandings, or because reality is rarely simple.  Hunger is a fun example of this.
