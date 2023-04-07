# Stock Trading Server

## Featuers

- Thread pool: we created a thread pool to increase the concurrency performance

- We used row-level lock when interacting with database instead of using mutex-lock in C++ code.


## Usage: 

### Run server
Enter docker-compose folder and type
```
docker-compose up
```

### functional test
In functional_test folder. We have test.sh under each subfolder, replace the `vcm-XXXXX.vm.duke.edu` to hostname of machine you are running the stock teading server.

Run this command in each sub-folder:
```
./test.sh
```

### Scalability test
Run the python file under the `scalability_test` folder



## Contents in functional test:

Each sub-folder have

-  `db_status.txt`:  all values in database after executed all these test cases under this subfolder
- `test.sh`: The shell script to run testcases.
- `response.xml`: All reponses returned after processed all these testcases. 

Test contents:

- basic_test: including create, cancel, regular orders, invalid create and query
- priority_buyer and priority_sell: sell and buy matching with muliple orders, chose the best fit
- partial_buy and partial_sell: partially matched sell or buy orders.
- test_create: test normal create and invalid creation request

