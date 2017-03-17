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
	return (ceil(((-1.0 / rate)*log(1.0 - u)) * 100000)); //working in units of 0.01ms
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
	return (rand() % numservers); 	// return range of 1 to number of hosts
}

int backofftimergen()
{
	return (rand() % 5 + 1); 	// arbitrary random countdown range (eg 1-5)
}

bool sortByArrive(event* lhs, event* rhs)
{
	return lhs->arrival < rhs->arrival;
}

int main()
{
	//initialize
	double lambda = 0;
	double mew = 0;
	double serverutil = 0;
	int NUMSERV = 0;

	double linktime = 0;

	double DIFS = 10;
	double SIFS = 5;

	event* curEvent = new event();
	event* generator = new event();
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
	vector< vector<event*> > SERV;

	vector<int> isfrozen(NUMSERV, 0); //maintain frozen status if waiting for ack
	vector<int> backoff(NUMSERV, 0); //list of backoff counts per server
	vector<int> attempts(NUMSERV, 0); //list of send attempts per server
	vector<int> prevlinktime(1, 0); //store duration of time in link of prev processed packet

	event* GEL[100000];

	//initialize both lists with all 0's
	for (int j = 0; j<NUMSERV; j++)
	{
		backoff.push_back(0);  //reference with vector.at(index)
		attempts.push_back(0);
	}


	//populate all servers with events
	for (int k = 0; k<NUMSERV; k++)
	{
		vector<event*> HOSTNUM; //empty row
		for (int i = 0;i<100000;i++)
		{
			if (i == 0)
			{
				curEvent->arrival = negexpdist(lambda);
				curEvent->depart = negexpdist(mew);
			}
			else
			{
				curEvent->arrival = (HOSTNUM[i - 1])->arrival + negexpdist(lambda);
				curEvent->depart = negexpdist(mew);
			}

			curEvent->target = target(NUMSERV);

			//prevent server from targeting itself -- DANGEROUS because prevents 1 server testing to debug, comment out to debug 1 server
			while (curEvent->target == k)
				curEvent->target = target(NUMSERV);

			curEvent->pktsize = negexpdistPKT(0.001); //used 0.001 to generate more realistic packet sizes

			generator = curEvent;

			HOSTNUM.push_back(generator); //add element to the row

			curEvent = new event();
		}//end packet generation for
		SERV.push_back(HOSTNUM);
	} //end NUMSERV for



	  //LOOP WILL START HERE TO SIMULATE PROCESS
	  //********************************************************************************
	for (int iterations = 0; iterations < 20; iterations++)
	{
		//generate departure times of heads and find earliest (min) packet ready to go
		double mindepart = 0;
		int mindepartindex = 0;
		char typecheck;
		event* minpacket = new event();

		for (int k = 0; k < NUMSERV; k++)
		{
			if (SERV[k].size() != 0)
			{
				//generate departure time
				/*if (SERV[k][0]->depart == 0)
				{*/
					SERV[k][0]->depart = SERV[k][0]->arrival + SERV[k][0]->depart + backoff[k];
				//}

				//find first packet ready to go
				if (mindepart == 0 || mindepart > SERV[k][0]->depart)
				{
					if (isfrozen[k] == 0)
					{
						mindepart = SERV[k][0]->depart;
						mindepartindex = k;
						minpacket = SERV[k][0];
					}
				}
			}
		}

		
		

		linktime = (minpacket->pktsize * 8) / (11 * 10 ^ 6) + DIFS;

		//pop the earliest one off front of host vector
		if (SERV[mindepartindex].size() != 0 )
		{
			SERV[mindepartindex].erase(SERV[mindepartindex].begin()); //pop off packet thats going into link
		}


		int backofftimehold = backoff[mindepartindex];
		//subtract all timers by the timer of the first ready to go and push back those ready to go times
		for (int a = 0; a<NUMSERV; a++)
		{
			if (SERV[a][0]->depart < minpacket->depart + linktime)
			{
				backoff[a] = backoff[a] - backofftimehold; //push earliest timer to 0 and decrement other backoffs accordingly
				SERV[a][0]->depart = minpacket->depart + linktime - SERV[a][0]->depart + backoff[a]; //delay by appropriate backoff
			}
		}

		//count send block
		if (minpacket->type == 'a')
		{
			isfrozen[minpacket->target] = 0;

			if(minpacket->depart + linktime <  SERV[minpacket->target][0]->depart)
			{
				SERV[minpacket->target][0]->depart = minpacket->depart + linktime;
			}

			if(minpacket->depart + linktime >=  SERV[minpacket->target][0]->depart)
			{
				if (attempts[minpacket->target] <=3)
				{
					SERV[minpacket->target][0]->depart=SERV[minpacket->target][0]->depart+500; //5ms
					attempts[minpacket->target] = attempts[minpacket->target] + 1;
					isfrozen[minpacket->target] = 1;
				}
				else
					isfrozen[minpacket->target] = 0;
			}
		}

		GEL[iterations] = minpacket; //move the packet that is processing into GEL

		//DEBUG
		cout << "Arrival: " << GEL[iterations]->arrival << "   Depart: "<< GEL[iterations]->depart << "    Type: " << GEL[iterations]->type << endl;


		//FREEZE STUFF
		//set freeze flag if waiting for ack
		if (GEL[iterations]->type == 's')
		{
			isfrozen[mindepartindex] = 1;
		}
		

		//DEBUG
		/*	cout << "iteration: " << iterations << endl;
		for(int z=0; z<SERV[0].size(); z++)
		{
		cout << SERV[0][z]->arrival<< " " << SERV[0][z]->type << SERV[0][z]->depart << endl;
		}*/


		//may need to prevent pop if waiting for ack not sure if alg covers it already or not
		//if (SERV[mindepartindex].size() != 0 )
		//{
		//	SERV[mindepartindex].erase(SERV[mindepartindex].begin()); //pop off packet thats going into link
		//}

		if (SERV[mindepartindex].size() != 0)
		{
			SERV[mindepartindex][0]->depart = SERV[mindepartindex][0]->arrival + negexpdist(mew) + 500; //generate processing time for new head
																										//the 500 is the 5ms freeze timer
		}




		//calculate time in link -> generate the ack packet if not an ack
		if (minpacket->type == 's')
		{
			
			ACK = new event(minpacket->depart + linktime, 0, 'a', mindepartindex); // create ack packet


			ACKset = ACK;

			//SERV[minpacket->target].push_back(ACKset);//put ack packet into target host
			//sort(SERV[minpacket->target].begin(),SERV[minpacket->target].end(), sortByArrive);//sort the ack into the host

			//find index to insert the ack packet by arrival time
			int searchminindex = 0;
			double searchmin = ACKset->arrival;

			while (searchmin > SERV[minpacket->target][searchminindex]->arrival)
			{
				searchminindex = searchminindex++;
			}
			//insert faster than sort

			SERV[minpacket->target].insert(SERV[minpacket->target].begin() + searchminindex, ACKset);

		}


		//push back all ready to go timers by overlap with linktime then set backoff counters
		for (int a = 0; a<NUMSERV; a++)
		{
			if (SERV[a][0]->depart < GEL[iterations]->depart + linktime)
			{
				if (backoff[a] == 0) //only generate new backoff timer if it was clear
				{
					backoff[a] = backofftimergen();
				}
				SERV[a][0]->depart = GEL[iterations]->depart + linktime - SERV[a][0]->depart;

				
			}
		}
		//DEBUG
			for (int a =0; a < NUMSERV; a ++)
	{
				cout << "DEBUG" << endl;
				cout << SERV[a][0]->depart << endl;
	}
 
	} //end of iterations for setting up GEL


	//DEBUG

	system("pause");
	return 0;
}


//ALGORITHM
//
//+have list of arrival times
//+generate processing time (ready to go) of each host (depart = ready to go, not actual departure time)
//
//__loop here__
//find earliest ready to go
//	+find first ready to go by calculating when backoff timer ends + ready to go time
//	+subtract all timers by the timer of the first ready to go and push back those ready to go times
//	+if: ready to go is ack and set unfreeze
//		if: ack ready to go + link time < target host head
//		then: target host head= ack ready to go + link time
//		if: ack ready to go + link time > target host head
//			while send counter <3
//			then: target host head = target host head + new freeze timer and send counter++
//
//+pop off earliest ready to go and calculate new host head ready to go
//+if: s packet
//+then: freeze host and generate ack
//+push back new host head ready to go by 5ms
//+generate link time
//+push back all ready to go timers by overlap with linktime then set backoff counters
//
