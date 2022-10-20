# CoFlop

An Implementation to slove the CoFlop optimization problem using Gurobi (C++ version)

1, The Gurobi optimizer as well as the license are avialble at ([https://www.gurobi.com/](https://www.gurobi.com/)).

2, After the installation of Gurobi optimizer, use the following command to comiple the code. 

    g++ -g -o chenglong line.cpp -I$GUROBI_HOME/include/ -L$GUROBI_HOME/lib/ -lgurobi_c++ -lgurobi81 -I/opt/software/boost/1.59.0/include/

3,  Three parameters are required for the implementation:

1. The number of nodes
2. The number of partitions on each node
3. The size of each data partition on each node (in the form of a matrix)

4, If any questions, please email to lcheng@ncepu.edu.cn
