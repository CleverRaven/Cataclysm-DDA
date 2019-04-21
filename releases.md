---
layout: page
title: Releases
permalink: /releases/
---

# Latest Stable
The most recent stable release is {{ site.github.latest_release.tag_name }} {{ site.github.latest_release.name }}.  We recommend stable for the most bug-free experience possible.  You can download it from the release page [here]({{ site.github.latest_release.html_url }}).

# Latest Experimental
Cataclysm: DDA is under constant development. As such, stable can lag behind with features. If you would like access to the bleeding edge of features, the links to the unstable experimental builds are below:
* [Windows Tiles build](http://dev.narc.ro/cataclysm/jenkins-latest/Windows/Tiles/)
* [Windows Terminal build](http://dev.narc.ro/cataclysm/jenkins-latest/Windows/Curses/)
* [Linux 64-bit Terminal build](http://dev.narc.ro/cataclysm/jenkins-latest/Linux_x64/Curses/)
* [Linux 64-bit Tiles build](http://dev.narc.ro/cataclysm/jenkins-latest/Linux_x64/Tiles/)
* [OSX Terminal build](http://dev.narc.ro/cataclysm/jenkins-latest/OSX/Curses/)
* [OSX Tiles build](http://dev.narc.ro/cataclysm/jenkins-latest/OSX/Tiles/)
* [Android build](http://dev.narc.ro/cataclysm/jenkins-latest/Android/Tiles/)

#### For historical interest, here are the previous "Stable" releases.

------------------

{% for release in site.github.releases %}
## {{ release.tag_name}} {{ release.name }}

{{ release.body }}

    {% unless release.assets == empty %}

#### {{ release.tag_name}} Download Links

        {% for asset in release.assets %}

* [{{ asset.name }}]({{ asset.browser_download_url }})

        {% endfor %}

    {% endunless %}

{% endfor %}
