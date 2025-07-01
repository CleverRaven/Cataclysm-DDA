var Twison = {

  /**
   * Extract the link entities from the provided text.
   *
   * Text containing [[foo]] would yield a link named "foo" pointing to the
   * "foo" passage.
   * Text containing [[foo->bar]] would yield a link named "foo" pointing to the
   * "bar" passage.
   *
   * @param {String} text
   *   The text to examine.
   *
   * @return {Array|null}
   *   The array of link objects, containing a `name` and a `link`.
   */
  extractLinksFromText: function(text) {
    var links = text.match(/\[\[.+?\]\]/g);
    const propRegexPattern = /\{\{((\s|\S)+?)\}\}((\s|\S)+?)\{\{\/\1\}\}/gm;
    if (!links) {
        return null;
    }

    // Split the text into lines
    var lines = text.split('\n');

    var processedLinks = []; // Array to store processed links

    lines.forEach(function(line) {
        var lineLinks = line.match(/\[\[.+?\]\]/g); // Find all links in the current line
        if (lineLinks) {
            lineLinks.forEach(function(link) {
                // Extract the link name by removing the surrounding [[ and ]]
                var topic = link.substring(2, link.length - 2);

                // Remove the link from the full line
                var lineWithoutLink = line.replace(link, '').trim();
                var processedLink = {topic: topic};
                var props = Twison.extractPropsFromText(line);
                var lineCleaned = lineWithoutLink.replace(propRegexPattern, '').trim();
                processedLink.text = lineCleaned;
                if (props) {
                  // Check if the props object contains an "effect" key
                  if (props.effect) {
                      processedLink.effect = props.effect; // Assign the "effect" key directly
                  } else if (props.condition){
                    processedLink.condition = props.condition
                  }
                  else {
                      processedLink.prop = props; // Fallback to assigning the entire props object
                  }
              }
  
                // Add the processed link to the array
                processedLinks.push(processedLink);
            });
        }
    });

    return processedLinks;
},
  /**
   * Extract the prop entities from the provided text.
   *
   * A provided {{foo}}bar{{/foo}} prop would yield an object of `{"foo": 'bar'}`.
   * Nested props are supported by nesting multiple {{prop}}s within one
   * another.
   *
   * @param {String} text
   *   The text to examine.
   *
   * @return {Object|null}
   *   An object containing all of the props found.
   */
  extractPropsFromText: function(text) {
    var props = {};
    var propMatch;
    var matchFound = false;
    const propRegexPattern = /\{\{((\s|\S)+?)\}\}((\s|\S)+?)\{\{\/\1\}\}/gm;

    while ((propMatch = propRegexPattern.exec(text)) !== null) {
      // The "key" of the prop, AKA the value wrapped in {{ }}.
      const key = propMatch[1];

      // Extract and sanitize the actual value.
      // This will remove any new lines.
      const value = propMatch[3].replace(/(\r\n|\n|\r)/gm, '');

      // We can nest props like so: {{foo}}{{bar}}value{{/bar}}{{/foo}},
      // so call this same method again to extract the values further.
      const furtherExtraction = this.extractPropsFromText(value);

      if (furtherExtraction !== null) {
        props[key] = furtherExtraction;
      } else {
        props[key] = value;
      }

      matchFound = true;
    }

    if (!matchFound) {
      return null;
    }

    return props;
  },

  /**
   * Convert an entire passage.
   *
   * @param {Object} passage
   *   The passage data HTML element.
   *
   * @return {Object}
   *   Object containing specific passage data. Examples include `name`, `pid`,
   *   `position`, etc.
   */
  convertPassage: function(passage) {
  	var dict = {text: passage.innerHTML};
    dict.topic = "talk_topic"; // Set default type
    ["name", "tags"].forEach(function(attr) {
      var value = passage.attributes[attr].value;
      if (value) {
        dict[attr] = value;
      }
    });

    if (dict.name) {
      dict.id = dict.name;
      delete dict.name; // Remove name to avoid confusion with id
    }
    if (dict.text) {
      // Split the text into lines
      var lines = dict.text.split('\n');
      var resultLines = [];
  
      // Iterate through the lines and stop when a line with a tag is found
      for (var i = 0; i < lines.length; i++) {
          var line = lines[i].trim();
          if (line.match(/\[\[.+?\]\]/)) { // Check if the line contains a tag (e.g., [[tag]])
              break; // Stop processing further lines
          }
          resultLines.push(line); // Add the line to the result
      }
  
      // Join the collected lines into a single string and assign to dynamic_line
      dict.dynamic_line = resultLines.join('\n').trim();
  }
    var links = Twison.extractLinksFromText(dict.text);
    if (links) {
      dict.responses = links;
    }
    if (dict.tags) {
      dict.tags = dict.tags.split(" ");
    }

    return dict;
	},

  /**
   * Convert an entire story.
   *
   * @param {Object} story
   *   The story data HTML element.
   *
   * @return {Object}
   *   Object containing processed "passages" of data.
   */
  convertStory: function(story) {
    var passages = story.getElementsByTagName("tw-passagedata");
    var convertedPassages = Array.prototype.slice.call(passages)
        .map(Twison.convertPassage)
        .filter(function(passage) {
            // Skip passages with name "TALK_DONE" or "TALK_NONE"
            return passage && passage.id !== "TALK_DONE" && passage.id !== "TALK_NONE";
        });

    var dict = {
      passages: convertedPassages
    };

    // Remove passages.text
    dict.passages.forEach(function(passage) {
      if (passage.text) {
        delete passage.text; // Remove the text key
      }
    });
    return dict.passages;
  },

  /**
   * The entry-point for converting Twine data into the Twison format.
   */
  convert: function() {
    var storyData = document.getElementsByTagName("tw-storydata")[0];
    var json = JSON.stringify(Twison.convertStory(storyData), null, 2);
    document.getElementById("output").innerHTML = json;
  }
}

window.Twison = Twison;