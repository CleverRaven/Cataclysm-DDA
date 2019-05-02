---
layout: page
title: Experimental Builds
permalink: /experimental/
---

# Latest Stable
The most recent stable release is {{ site.github.latest_release.tag_name }} {{ site.github.latest_release.name }}.  We recommend stable for the most bug-free experience possible.  You can download it from the [Releases Page]({{ site.baseurl }}{% link releases.md %}).

# Latest Experimental
Cataclysm: DDA is under constant development. As such, stable can lag behind with features. If you would like access to the bleeding edge of features and any bugs that may come along with them, you can download the latest experimental builds below:

------------------

{% for release in site.github.releases %}

    {% if release.prerelease %}

        {% unless release.assets == empty %}

## {{ release.name }}

{{ release.body }}

Timestamp: {{ release.published_at }}

            {% for asset in release.assets %}

* [{{ asset.name }}]({{ asset.browser_download_url }})

            {% endfor %}

        {% endunless %}

    {% endif %}

{% endfor %}
