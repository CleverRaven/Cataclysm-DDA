---
layout: page
title: Releases
permalink: /releases/
---
{% for release in site.github.releases %}
  {{ release.name }}
------------------

  {{ release.body }}
{% endfor %}