#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>


using namespace std;

class event
{
public:
	double arrival;
	double depart;
	char type;
	int target;
	double pktsize;

	event();
	event(double, double, int tar);
	event(double, double, char a, int tar); //ack packet
};

event::event()
{
	arrival = 0;
	depart = 0;
	type = 's';
	target = NULL;
	pktsize = 0;
}

event::event(double arrive, double departure, int tar)
{
	arrival = arrive;
	depart = departure;
	type = 's';
	target = tar;
	pktsize = 0;
}

event::event(double arrive, double departure, char a, int tar)
{
	arrival = arrive;
	depart = departure;
	type = 'a'; //ack packet
	target = tar;
	pktsize = 64;
}

double negexpdist(double rate)
{
	double u;
	u = (rand() / (RAND_MAX + 1.0)); //drand48() not happy in visual studio, this equation models drand48()
	return ((-1.0 / rate)*log(1.0 - u));
}

double negexpdistPKT(double rate)
{
	double u;
	double v = 1545;

	while (v > 1544)
	{
	u = (rand() / (RAND_MAX + 1.0)); //drand48() not happy in visual studio, this equation models drand48()
	v = ((-1.0 / rate)*log(1.0 - u));
	}
	return ceil(v);
}

int target(int numservers)
{
	return (rand() % numservers + 1); 	// return range of 1 to number of hosts
}

bool sortByDepart(event* lhs, event* rhs) 
{ 
	return lhs->depart < rhs->depart;
}

int main()
{
	//initialize
	double lambda = 0;
	double mew = 0;
	double serverutil = 0;
	int NUMSERV = 0;

	double linktime = 0;

	double DIFS = 0.0001;
	double SIFS = 0.00005;

	event* curEvent = new event();
	event* previousEvent = new event();
	event* ACK = new event();
	event* ACKset = new event();

	cout << "lamda: ";
	cin >> lambda;
	cout << endl;

	cout << "mew: ";
	cin >> mew;
	cout << endl;

	cout << "Number of Servers: ";
	cin >> NUMSERV;
	cout << endl;

	//vector of vectors, with each list representing a server
	vector< vector<event*> > SERV (NUMSERV, vector<event*>(1));

	vector<int> backoff; //list of backoff counts per server
	vector<int> attempts; //list of send attempts per server

	event* GEL[100000];

	//initialize both lists with all 0's
	for (int j = 0; j<NUMSERV; j++)
	{
		backoff.push_back(0);  //reference with vector.at(index)
		attempts.push_back(0);
	}


	//100000 iteration(events) -> populate all servers with event starting times
	for (int i = 0;i<100000;i++)
	{
		vector<event*> HOSTNUM; //empty row
		for (int k = 0; k<NUMSERV; k++)
		{
			if (i == 0)
			{
				curEvent->arrival = negexpdist(lambda);
			}
			else
			{
				curEvent->arrival = (SERV[k][i-1])->arrival + negexpdist(lambda);
			}

			curEvent->target = target(NUMSERV);
			curEvent->pktsize = negexpdistPKT(lambda);

			HOSTNUM.push_back(curEvent); //add element to the row
		}//end packet generation for
		SERV.push_back(HOSTNUM);
	} //end NUMSERV for
	
	//LOOP WILL START HERE TO SIMULATE PROCESS
	for (int iterations = 0; iterations < 100000; iterations++)
	{
		//generate departure times of heads and find earliest (min) packet ready to go
		double mindepart = 0;
		int mindepartindex = 0;
		event* minpacket = new event();

		for (int k = 0; k < NUMSERV; k++)
		{
			if (SERV[k][0]->depart == 0)
			{
				SERV[k][0]->depart = SERV[k][0]->arrival + negexpdist(mew);
			}

			//find first packet ready to go
			if (mindepart == 0 || mindepart > SERV[k][0]->depart)
			{
				mindepart = SERV[k][0]->depart;
				mindepartindex = k;
				minpacket = SERV[k][0];
			}
		}

		GEL[iterations] = minpacket; //move the packet that is processing into GEL

		SERV[mindepartindex].erase(SERV[mindepartindex].begin()); //pop off packet thats going into link

		SERV[mindepartindex][0]->depart = SERV[mindepartindex][0]->arrival + negexpdist(mew); //generate processing time for new head

		//calculate time in link generate the ack packet if not an ack
		if (minpacket->type == 's')
		{
			linktime = (minpacket->pktsize * 8) / (11 * 10 ^ 6);
			ACK = new event(minpacket->depart + linktime + DIFS, 0, 'a', mindepartindex); // create ack packet
			ACKset = ACK;
			SERV[minpacket->target].push_back(ACKset);//put ack packet into target host
			sort(SERV[minpacket->target].begin(),SERV[minpacket->target].end(), sortByDepart);//sort the ack into the host
		}
		else //ack packet - no need to generate ack packet
		{
			linktime = (64 * 8) / (11 * 10 ^ 6);
		}

		


		//still need to:
		//fix vector
		//see if time in link delays anyone
		//freeze when waiting for ack
		//count resend attempts
		//countdown timer
		//clock


	} //end of iterations loop/simulation

	system("pause");
	return 0;
}



