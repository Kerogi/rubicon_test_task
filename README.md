# rubicon_test_task
rubicon test task

Dependencies:

* Cmake
* Boost libs >=1.58
* Libcurl

On debian it should be availabel in default repo

`apt-get install cmake libboost-all-dev libcurl4-openssl-dev `

Building:

Just call: `cmake . && make`

Running:

Call shell script `run_nweb_and_3_nodes.sh` from project root after succesfull build.

It will run a 4 instances of the server on ports 12301-12304 of simple leaf servers and 1 instance of proxy server at port 12300.

Test:

In browser goto address:

`localhost:12300/multiproxy?query=<restaurant type>`

Where restaurant **type** is something like *Italian, European* etc. (try more)

This request will give agregated search results from all 4 servers.

Or you could add to adress parameter `&dest=<peer server name>,<...>` to specify server(s) to proxy search.

You can search same query individually on these peer nodes by browser like:

`localhost:1230x/restaurant?query=<restaurant type>`

Note search could match a partial prefix, but it is case sensitive.

For reference of *servers names* ans *restaurant types* see **xml** files in project.
