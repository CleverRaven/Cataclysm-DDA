#!/usr/bin/env sh
MARKDOWN_CMD="python3 -m markdown -o html5 -n <../README.md"

cat > ../README.html <<EOF
<html>
<head>
<meta content='text/html; charset=utf-8' http-equiv='Content-Type'>
<title>Cataclysm: The Dark Days Ahead READ ME</title>
<style>
EOF

cat >> ../README.html < ../readme.css

cat >> ../README.html <<EOF
</style>
</head>
<body>
<div id='container'>
EOF

$MARKDOWN_CMD >> ../README.html

cat >> ../README.html <<EOF
</div>
</body>
</html>
EOF
