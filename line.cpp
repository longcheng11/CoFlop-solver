/* Copyright 2016, Gurobi Optimization, Inc. */

/* Facility location: a company currently ships its product from 5 plants
 to 4 warehouses. It is considering closing some plants to reduce
 costs. What plant(s) should the company close, in order to minimize
 transportation and fixed costs?

 Based on an example from Frontline Systems:
 http://www.solver.com/disfacility.htm
 Used with permission.

 The code below is based on the "Facility" example from Gurobi.
 @LongCheng

 */

#include "gurobi_c++.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <time.h>
#include <boost/tokenizer.hpp>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
	GRBEnv *env = 0;
	GRBVar **assignment = 0;
	GRBVar w;

	int nodeCt = 0;
	clock_t start, finish;

	try
	{

		// Number of plants and warehouses
		const int nNode = atoi(argv[1]);
		cout << "number of nodes is " << nNode << endl;

		//the number of total keys
		int total = 0;

		//default is 15*number of nodes
		const int nPartition = atoi(argv[2]);;
		int h[nNode][nPartition];

		// Transportation costs per thousand units
		typedef boost::tokenizer< boost::char_separator<char> > Tokenizer;
		boost::char_separator<char> sep(",");
		string line;

		//read comm matrix
		ifstream rfile;
		string file2 = string(argv[3]);
		rfile.open(file2.c_str());
		int cNode = 0;
		int cPartition = 0;
		if (rfile)
		{
			while (getline(rfile, line))
			{
				Tokenizer info(line, sep);
				for (Tokenizer::iterator it = info.begin(); it != info.end(); ++it)
				{
					h[cNode][cPartition] = strtod(it->c_str(), 0);
					total += h[cNode][cPartition];
					cPartition++;
				}
				cNode++;
				cPartition = 0;
			}
		}
		else
		{
			cout << "\n no file!" << endl;
		}
		rfile.close();
		cout << "reading Comm Matrix is done!" << endl;

		double upperbound = total;

		// Model
		env = new GRBEnv();
		GRBModel model = GRBModel(*env);
		model.set(GRB_StringAttr_ModelName, "coflop_assignment");
		model.getEnv().set(GRB_IntParam_Threads, 1);

		model.update();

		assignment = new GRBVar *[nNode];
		for (int n = 0; n < nNode; n++)
		{
			assignment[n] = model.addVars(nPartition, GRB_BINARY);
			model.update();
			for (int p = 0; p < nPartition; p++)
			{
				ostringstream vname;
				vname << nNode << "." << nPartition;
				assignment[n][p].set(GRB_StringAttr_VarName, vname.str());
			}
		}

		w = model.addVar(0, upperbound, NULL, GRB_CONTINUOUS, "w");
		model.update();

		//Objective function: minimize w
		GRBLinExpr obj = 0;
		obj = obj + w;
		model.setObjective(obj, GRB_MINIMIZE);

		//the constraints
		for (int n = 0; n < nNode; n++)
		{
			//the initial value of each s_i and r_i at each node
			GRBLinExpr sum_of_sent = 0;
			GRBLinExpr sum_of_received = 0;
			nodeCt++;
			for (int p = 0; p < nPartition; p++)
			{
				sum_of_sent = sum_of_sent + h[n][p] * (1 - assignment[n][p]);

				double sum_of_received_each_partition = 0;
				for (int k = 0; k < nNode; k++)
				{
					if (k != n)
					{
						sum_of_received_each_partition += h[k][p];
					}
				}
				sum_of_received = sum_of_received +
								  assignment[n][p] * sum_of_received_each_partition;
			}
			ostringstream cname_sent;
			cname_sent << "sent_packets_of_" << n;
			model.addConstr(w >= sum_of_sent, cname_sent.str());

			ostringstream cname_received;
			cname_received << "received_packets_of_" << n;
			model.addConstr(w >= sum_of_received, cname_received.str());
		}

		for (int p = 0; p < nPartition; p++)
		{
			GRBLinExpr unique = 0;
			for (int n = 0; n < nNode; n++)
			{
				unique += assignment[n][p];
			}

			ostringstream cname_unique;
			cname_unique << "unique_of_" << p;
			model.addConstr(unique == 1, cname_unique.str());
		}

		// Update model to integrate new variables
		model.update();

		// Use barrier to solve root relaxation
		//model.getEnv().set(GRB_IntParam_Method, GRB_METHOD_BARRIER);

		// Solve
		model.optimize();

		int status = model.get(GRB_IntAttr_Status);

		if ((status == GRB_INF_OR_UNBD) || (status == GRB_INFEASIBLE) ||
			(status == GRB_UNBOUNDED))
		{
			cout << "\n The model cannot be solved "
				 << "because it is infeasible or unbounded"
				 << "error is " << status << endl;
			return status;
		}

		//generate comm matrix
		int comm[nNode][nNode];
		for (int i = 0; i < nNode; i++)
		{
			for (int j = 0; j < nNode; j++)
			{
				comm[i][j] = 0;
			}
		}
		for (int p = 0; p < nPartition; p++)
		{
			int des = 0;
			for (int n = 0; n < nNode; n++)
			{
				if (assignment[n][p].get(GRB_DoubleAttr_X) == 1.0)
				{
					des = n;
				}
			}

			for (int n = 0; n < nNode; n++)
			{
				if (n != des)
				{
					comm[n][des] += h[n][p];
				}
			}
		}

		//output the assignment
		for (int n = 0; n < nNode; n++){
			for (int p = 0; p < nPartition; p++){
				cout << assignment[n][p].get(GRB_DoubleAttr_X) << ",";
			}
			cout << "\n";
		}

		//output comm matrix
		for (int j = 0; j < nNode; j++)
		{
			for (int i = 0; i < nNode - 1; i++)
			{
				cout << comm[j][i] << ",";
			}
			cout << comm[j][nNode - 1] << "\n";
		}
		
		cout << "comm matrix has been generated\n " << endl;
		double none_migrated_data = 0;
		for (int n = 0; n < nNode; n++)
		{
			for (int p = 0; p < nPartition; p++)
			{
				if (assignment[n][p].get(GRB_DoubleAttr_X) == 1.0)
				{
					none_migrated_data += h[n][p];
				}
			}
		}

		cout << "\nscheduling time is " << model.get(GRB_DoubleAttr_Runtime) << " secs" << endl;
		cout << "tranferred/total is " << none_migrated_data << " " << upperbound << endl;
		cout << "data locality is " << none_migrated_data / upperbound << endl;
		cout << "max sent/rev is " << model.get(GRB_DoubleAttr_ObjVal) << endl;
	}
	catch (GRBException e)
	{
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	}
	catch (...)
	{
		cout << "Exception during optimization" << endl;
	}

	for (int n = 0; n < nodeCt; n++)
	{
		delete[] assignment[n];
	}

	delete[] assignment;

	delete env;
	return 0;
}
