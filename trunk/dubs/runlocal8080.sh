FIRSTDIR=`pwd`
cd ..
${FIRSTDIR}/bin/dubs.wt --docroot . --http-address 0.0.0.0 --http-port 8080 -c auth_config.xml
cd ${FIRSTDIR}
