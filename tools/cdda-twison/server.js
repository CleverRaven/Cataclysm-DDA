var Express = require('express')

var port = process.env.PORT || 3000;

var app = Express()
app.use(Express.static('./dist'))
var server = app.listen(port, function () {
  console.log("Serving story format at http://localhost:" + port + "/format.js\n");

  require('./watch.js')
});
