---
layout: page
title: Releases
permalink: /releases/
---

# Latest Stable
The most recent stable release is {{ site.github.latest_release.tag_name }} {{ site.github.latest_release.name }}. We recommend stable for the most bug-free experience possible. You can download it from the links below. Note: downloads with 'Curses' in the name include a version that can be played in the console and has purely text-based display. Downloads with 'Tiles' in the name includes a version of the game that has sprite-based graphics. It can also be downloaded from many [package repositories]({{ site.baseurl }}{% link pages/packaging.md %}).

# Latest Experimental
Cataclysm: DDA is under constant development. As such, stable can lag behind with features. If you would like access to the bleeding edge of features and any bugs that may come along with them, you can download the [latest experimental builds]({{ site.baseurl }}{% link pages/experimental.md %}).

------------------

{% for release in site.github.releases %}

    {% unless release.prerelease %}

## {{ release.tag_name}} {{ release.name }}

{{ release.body }}

        {% unless release.assets == empty %}

#### {{ release.tag_name}} Download Links

            {% for asset in release.assets %}

* [{{ asset.name }}]({{ asset.browser_download_url }})

            {% endfor %}

        {% endunless %}

    {% endunless %}

{% endfor %}
