#
# Regular cron jobs for the salut package.
#
0 4	* * *	root	[ -x /usr/bin/salut_maintenance ] && /usr/bin/salut_maintenance
