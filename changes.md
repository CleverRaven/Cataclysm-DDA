---
layout: page
title: Changes
permalink: /changes/
---
{% for pull in site.github.pulls %}
  {{ pull.title }}
------------------

  {{ pull.body }}
{% endfor %}
