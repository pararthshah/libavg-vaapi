#/bin/bash
epydoc --html -v -o reference --name libavg --url ../index.php --docformat epytext --no-frames --graph classtree --css epydoc_libavg.css --no-private --graph-font="lucida console" libavg
#epydoc --html -o reference -n libavg -u ../index.html --no-private --docformat plaintext --no-frames  /usr/local/lib/python2.4/site-packages/libavg/avg.so
#man ../man/avg.1 | man2html -title avg avg > man_avg.html
#man ../man/avgrc.5 | man2html -title avgrc avgrc > man_avgrc.html

