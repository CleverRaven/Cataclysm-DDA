#!/usr/bin/env Rscript

# Read and transpose (rotate) TSV data from STDIN
d = read.table(file("stdin"), sep='\t', header=T, check.names=F)
d = setNames(data.frame(t(d[,-1])), d[,1])

png("output.png", width=800, height=720, pointsize=18)
par(mar=c(5.5,5.5,3,3))
par(bg = 'gray95')

palette <- 1:length(d)

matplot(0:(length(d[[1]])-1),d, type='l', lty='solid', lwd=5, xlab='Range', ylab='Mean damage/shot', xaxs='i', yaxs='i', ylim=c(floor(min(d)),ceiling(max(d))), axes=F, mgp=c(3.5,0,0), font.lab=2, col.lab='gray10', cex.lab=1.2, col=palette)
axis(side = 1, tck=-0.08, lwd=0, col='gray65', col.axis='gray10', lwd.ticks=2, mgp=c(0,2,0), font=2)
axis(side = 2, tck=-0.08, lwd=0, col='gray65', col.axis='gray10', lwd.ticks=2, mgp=c(0,2,0), font=2)
axis(side = 3, tck=-0.08, lwd=0, col='gray65', col.axis='gray10', lwd.ticks=2, labels=F)
axis(side = 4, tck=-0.08, lwd=0, col='gray65', col.axis='gray10', lwd.ticks=2, labels=F)
grid(NULL, NULL, lty = 'solid', lwd=2, col='gray65')

# Repeat plot so grid lines are positioned behind data series
par(new=T)
matplot(0:(length(d[[1]])-1),d, type='l', lty='solid', lwd=3, xlab='Range', ylab='Mean damage/shot', xaxs='i', yaxs='i', ylim=c(floor(min(d)),ceiling(max(d))), axes=F, mgp=c(3.5,0,0), font.lab=2, col.lab='gray10', cex.lab=1.2, col=palette)

legend("topright", legend=colnames(d), inset=0.02, lwd=3, bg='gray90', box.lwd=0, text.col='gray10', col=palette)
