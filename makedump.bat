exe -0 em.x @em_dump.rc -d
mv em.dmp em1.dmp
exe -98304 em.x @em_dump.rc -d
mv em.dmp em2.dmp
makex em1.dmp em2.dmp emx.x
