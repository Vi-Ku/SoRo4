﻿#include "autonomous.h"
#include <list>
#include <queue>
#include <set>
#include <unistd.h>

#define PI 3.14159265

//this class assumes that the stuff to get the gpsHeading, the stuff to actually make the rover move, and everything needed for GeneratePath is available from another class.

//Higher value means more avoidance from the algorithm
const double SearchAlgorithm::DISTWEIGHT = 1.0; //Weight given to the distance between two nodes when calculating cost
const double SearchAlgorithm::UPWEIGHT = 1000.0; //Weight given to the difference in elevation when going up
const double SearchAlgorithm::DOWNWEIGHT = 1000.0; //Weight given to the difference in elevation when going down
Cell** SearchAlgorithm::map; //Matrix of Cell objects
int SearchAlgorithm::maxx; //max x-value on the map
int SearchAlgorithm::maxy; //max y-value on the map
bool SearchAlgorithm::initialized = false; //are the map and max values initialized?

volatile double angle = 0.0; //current angle of travel from the horizontal. Sign is reversed from what is expected

void SearchAlgorithm::initializeMap(Cell ** map, int maxx, int maxy)
{
	//Change the static class members to their provided values
	SearchAlgorithm::map = map;
	SearchAlgorithm::maxx = maxx;
	SearchAlgorithm::maxy = maxy;
	initialized = true;
}

std::list<Cell> SearchAlgorithm::findPath(Cell source, Cell dest)
{
	//if the map is not initialized, print out a list containing only source and dest
	if (!initialized) {
		std::list<Cell> out;
		out.push_front(dest);
		out.push_front(source);
		return out;
	}

	//Determine the x and y values of the source and destination from their latitude and longitude

	//Determine the difference in latitude between the first two rows
	double latDiff = map[0][1].lat - map[0][0].lat; //Should be negative because 0 lat is equator

	//Find y coordinates
	int sourcey = round((source.lat - map[0][0].lat) / latDiff);
	int desty = round((dest.lat - map[0][0].lat) / latDiff);

	//Determine the difference in longitude between the first two columns
	double lngDiff = map[1][0].lng - map[0][0].lng; //should be negative becasuse longitude proceeds east to west in NA

	//Find x coordinates
	int sourcex = round((source.lng - map[0][0].lng) / lngDiff);
	int destx = round((dest.lng - map[0][0].lng) / lngDiff);

	//if any of the coordinates are out of bounds of the map, return the error cell
	if ((sourcey > maxy) || (sourcey < 0) || (desty > maxy) || (desty < 0) || (sourcex > maxx) || (sourcex < 0) || (destx > maxx) || (destx < 0)) {
		Cell error(-1.0, -1.0, 0.0);
		std::list<Cell> out;
		out.push_front(error);
		return out;
	}

	//Create the source node and add it to the open list
	std::priority_queue<Node, std::vector<Node>, compareNodes> open; //Create open, closed, and register lists
	std::set<Node, compareNodes2> closed;

	Node sourceNode(sourcex, sourcey, nullptr, 0.0, 0.0);
	Node destNode(destx, desty, &sourceNode, 0.0, 0.0); //CHNG 10/3: changed 'nullptr' to '&sourceNode'. This will change no-path output
	open.push(sourceNode);

	//Loop while there are still elements in the open list
	while (!open.empty()) {
		//Add the best element in the open list to the closed list
		Node * current = new Node(open.top()); //CHNG 10/3: to prevent a weird bug that made each node its own parent
		open.pop();

		//if that element is the destination, we're done with the loop
		if (*current == destNode) {
			destNode.parent = current->parent;
			break;
		}

		//if the current node is not in the closed list, add it, and add its neighbors to the open list
		std::pair<std::set<Node, compareNodes2>::iterator, bool> inserted = closed.insert(*current);
		if (inserted.second) {
			for (Node neighbor : getNeighbors(*current, destx, desty)) {
				open.push(neighbor);
			}
		}

		delete current;
	}

	//create output list
	std::list<Cell> out;
	Node * interest = new Node(destNode);

	//ascend the parent tree, adding the corresponding GPS coordinates until we reach the source
	do {
		Cell cell = map[interest->x][interest->y];

		Cell pair = new Cell(); //CHNG 10/5: dynamically allocated array to avoid the overwriting problem
		pair.lat = cell.lat;
		pair.lng = cell.lng;

		out.push_front(pair);

		interest = interest->parent;
	} while (interest != nullptr);

	//CHNG 10/3: interest interest might be assigned to a null pointer, so changed it to a pointer

	//return output list
	return out;
}

std::list<SearchAlgorithm::Node> SearchAlgorithm::getNeighbors(Node& current, int destx, int desty) {
	std::list<Node> out;

	//for each neighbor, excluding the current node and any neighbors that go out of map bounds
	for (int x = current.x - 1; x <= current.x + 1; x++) { //CHNG 'x < current.x + 1' to 'x <= current.x + 1'
		for (int y = current.y - 1; y <= current.y + 1; y++) { //CHNG 'y < current.y + 1' to 'x <= current.y + 1'
			if (x < 0 || y < 0 || x >= maxx || y >= maxy || (x == current.x && y == current.y)) {
				continue;
			}

			//construct a new node for each neighbor and add it to the list
			double newG = getGCost(current, x, y);
			double newF = newG + getHeuristic(destx, desty, x, y);

			Node * parent = new Node(current);
			Node * neighbor = new Node(x, y, parent, newG, newF);

			out.push_front(*neighbor);
		}
	}

	return out;
}

double SearchAlgorithm::getGCost(SearchAlgorithm::Node current, int x, int y) {
	//if the two nodes are adjacent, distance is 1, if they are diagonal, it is 1.4
	double distance = (abs((current.x + current.y) - (x + y)) == 1) ? 1.0 : 1.4;
	double gradientDiff = map[current.x][current.y].gradient - map[x][y].gradient;

	double gradientVal = 0.0;
	//Weight the gradient differently depending on if we are going up or down
	if (gradientDiff < 0) { //we're going up
		gradientVal = abs(gradientDiff) * UPWEIGHT;
	}
	else {
		gradientVal = gradientDiff * DOWNWEIGHT;
	}

	//return the cost so far plus the cost to move to the new node
	return current.g + (DISTWEIGHT * distance + gradientVal);
}

double SearchAlgorithm::getHeuristic(int destx, int desty, int x, int y) {
	//return the Manhattan distance between the point of interest and the destination point
	return (abs(destx - x) + abs(desty - y));
}

Autonomous::Autonomous()
{
    qInfo() << "library link test";

    mainLoop();
}

//return the speeds that the wheels need to move at to get to the next coordinate
std::vector<double> Autonomous::getWheelSpeedsValues(double amountOff, double baseSpeed)
{
    std::vector<double> PIDValues(2);

    if(baseSpeed > 0)
    {
        //this formula is still almost certainly going to need to be adjusted
        PIDValues[0] = speed + speed * (1.045443e-16 + 0.00001087878*amountOff - 1.0889139999999999e-27*pow(amountOff, 2) + 7.591631000000001e-17*pow(amountOff, 3) - 7.105946999999999e-38*pow(amountoff, 4);
        PIDValues[1] = speed - speed * (1.045443e-16 + 0.00001087878*amountOff - 1.0889139999999999e-27*pow(amountOff, 2) + 7.591631000000001e-17*pow(amountOff, 3) - 7.105946999999999e-38*pow(amountoff, 4);
    }

    else
    {
        PIDValues[0] = speed - speed * (1.045443e-16 + 0.00001087878*amountOff - 1.0889139999999999e-27*pow(amountOff, 2) + 7.591631000000001e-17*pow(amountOff, 3) - 7.105946999999999e-38*pow(amountoff, 4);
        PIDValues[1] = speed + speed * (1.045443e-16 + 0.00001087878*amountOff - 1.0889139999999999e-27*pow(amountOff, 2) + 7.591631000000001e-17*pow(amountOff, 3) - 7.105946999999999e-38*pow(amountoff, 4);
    }

    return PIDValues;
}

//Returns a list of cells from the rover's current location to the specified destination
std::list<Cell> Autonomous::GeneratePath(Cell dest)
{
	Cell source;
	source.lat = GPSObject.latitude;
	source.lng = GPSObject.longitude;
	source.gradient = 0.0;

	return findPath(source, dest);
}

//impliment much later
bool Autonomous::isThereObstacle()
{

}

//Simply backs up, turns for a bit and then drives forward to before resuming normal operations if the robot is stuck or sees an obstacle
void Autonomous::avoidObstacle()
{
    //backs up for 5 seconds
    mySocket.sendUDP(0, 0, 0, -speed, -speed, 0, 0, -speed);
    usleep(5000);

    //turns for a few seconds to hopefully avoid the obstacle
    mySocket.sendUDP(0, 0, 0, -speed, speed, 0, 0, 0);
    usleep(5000);

    //drive forward a bit and continue(?)
    mySocket.sendUDP(0, 0, 0, speed, speed, 0, 0, speed);
    usleep(5000);
}

//Someone needs to port the working python code to track the tennis ball into here
void Autonomous::FindTennisBall()
{

}

//returns the difference between the current angle to the horizontal and the desired angle to reach the next cell
double Autonomous::getAngleToTurn(Cell next)
{
    double latitude = pos_llh.lat;
    double longitude = pos_llh.lon;
    double target = std::atan((next.lng - longitude) / (next.lat - latidude)) * 180 / PI; //NOTE: angle sign is opposite of standard
    return target - angle;
}

//This is meant to be run as a thread the whole time the autonomous program is running.
//This finds the angle that the rover is at and if the rover is stuck. This will be inaccurate if the rover does a pivot turn
//The way that it finds if the rover is stuck is if it is in the EXACT same position 6 times in a row (3 seconds)
//TODO add the accelerometer information here (probably(?))
void Autonomous::updateStatus()
{
    double longitude = pos_llh.lon;
    double latitude = pos_llh.lat;

    while(threadsRunning)
    {
        longitude = pos_llh.lon;
        latitude = pos_llh.lat;

        //if the robot hasn't moved
        if(lastLatitude == latitude && lastLongitude == longitude)
        {
            timesStuck++;
            if(timesStuck >= 6)
            {
                isStuck = true;
            }
        }

        else
        {
            isStuck = false;
            timesStuck = 0;
            angle = std::atan((longitude - lastLongitude) / (latitude - lastLatitude)) * 180 / PI; //NOTE: sign is opposite of usual

            lastLatitude = latitude;
            lastLongitude = longitude;
            usleep(500000); //this sleep may need to be increased or decreased depending on how often we want the rover to update its angle
        }
    }
}

//This needs to be implemented as a GUI function where we can input the next set of coordinates that the people tell us the tennis ball is
Cell Autonomous::inputNextCoords()
{

}

//Goes through all of the coordinates that we need to travel through
//Calls drive for the robot to smoothly reorient itself to from one node to the next
int Autonomous::MainLoop()
{
	//placeholder
	parseMap();

    //this can probably be done better by someone who is better at cpp than me
    //this is just so we can tell the robot to stop driving
    std::vector<double> killVector(2);
    killVector[0] = -1;
    killVector[1] = -1;

    timesStuck = 0;
    isStuck = false;

    lastLatitude = pos_llh.lat;
    lastLongitude = pos_llh.lon;

    std::vector<double> nextCords = inputNextCords(); //variable to hold the next coords that we need to travel to. Immediately calls the method to initialize them

    threadsRunning = true;
    std::thread angleThread(updateStatus);
    while(nextCords != killVector) //checks to make sure that we don't want to stop the loop
    {
        std::list<Cell> path = generatePath(nextCords); //TODO generates the path to the given set of coords
		std::list<Cell>::iterator it = path.begin();

         //loops through each of the coordinates to get to the next checkpoint        
        while(*it != nextCords) //travels to the next set of coords. 
        {
            while(ListOfCoordsToNextCheckpoint[i] != CurrentGPSHeading) //travels to the next set of coords. CurrentGPSHeading needs to be the range of coordinates that we want the rover to reach
            {
                if(isThereObstacle() || isStuck)
                {
                    avoidObstacle();
                }
                else //drives trying to get to the next checkpoint
                {
                    //find the angle that the robot needs to turn to to be heading in the right direction to hit the next coords
                    double angleToTurn = getAngleToTurn();

                    std::vector<double> speeds = getWheelSpeedValues(angleToTurn, speed);
                    mySocket.sendUDP(0, 0, 0, speeds[0], speeds[1], 0, 0, (speeds[0] + speeds [1]) / 2);

                    usleep(500000); //lets it drive for 500ms before continuing on
                    it++;
                }
            }
        }
        
        //once arrives to the checkpoint
        FindTennisBall();
        nextCords = inputNextCords(); //gets the next set of coords
    }

    threadsRunning = false;
    updateAngle.join();

    cout << "We win!" << endl;
    return 0;
}


