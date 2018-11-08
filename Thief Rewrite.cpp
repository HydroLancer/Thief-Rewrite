// Thief Rewrite.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <math.h>
#include <sstream>
#include "CVector3cut.h"
#include "CMatrix4x4cut.h"

using namespace tle;

I3DEngine* myEngine = New3DEngine(kTLX);

//function definitions
//comments on the actual functions (at the bottom)

//thief related
void characterMovement(IModel* character, float frameTimer);	//WASD, etc
void floorHandler(IModel* thief, IModel* square[]);				//Step on the red to make the floor scream

//guard related
void guardFacingVector(IModel* guard, IModel* thief);													//gets guard's facing vector
void guardPatrol(IModel* guard, IModel* waypoint[]);													//guard's patrol route - uses dummies for waypoints, sphere to point collision
float guardToThiefDistance(IModel* guard, IModel* character);											//self explanatory - distance between thief & guard
void detectionHandler(float distance, enum Thief noise, float dotProduct, enum Guard &guardState);		//logic! what happens when - sets enum states for the guard (Idle, alert, dead)
float guardLeftorRight();
void Look(IModel* guard);

//Maffs
float dotProduct(IModel* guard, IModel* thief);		//gives the ""Angle"" between guard's FV & thief -> guard vector

//real time debuggin'
void textOutput(IFont* font, float angle, float distance, enum Guard guardState, enum Thief thiefNoise, enum ThiefMovement movementType);

//some variables
float gameSpeed;

//Thief Movement Speeds
float thiefCreep = 10.0f;
float thiefWalk = 20.0f;
float thiefRun = 30.0f;
float turnSpeed = 45.0f;

//Guard Movement Speeds & other thingies
float guardPatrolling = 5.0f;
float guardChasing = 10.0f;
float side = 0.0f;

int currentWaypoint = 0;

float dotProductResult = 0.0f;
float distanceToThief = 0.0f;

//Maths-y Things
CVector3 facingVector;		//stores current facing vector of the guard
CVector3 guardLocalX;
CVector3 vectorToThief;

CVector3 thiefPosition;		//I guess it stores the current pos of the MC
CVector3 guardPosition;		//as above but for guard

//controls
const EKeyCode forwards = Key_W;
const EKeyCode backwards = Key_S;
const EKeyCode turnLeft = Key_A;
const EKeyCode turnRight = Key_D;
const EKeyCode gameExit = Key_Escape;
const EKeyCode run = Key_1;
const EKeyCode walk = Key_2;
const EKeyCode creep = Key_3;

//states
enum Guard {idle, alert, dead};
enum Thief {quiet, noisy};
enum ThiefMovement {creeping, walking, running};

Guard guardState;				//guard
Thief noiseLevel;				//thief
ThiefMovement movementLevel;	//movementtype


void main()
{
	myEngine->StartWindowed();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder("C:\\ProgramData\\TL-Engine\\Media");
	//myEngine->AddMediaFolder("E:\\Uni Stuff!\\2nd Year\\Game Dev 1\\Labs\\Thief-Rewrite\\models");	//Home Desktop
	//myEngine->AddMediaFolder("C:\\Users\\Hydro\\Desktop\\Uni Lab Stuff\\Thief-Rewrite\\models");	//Laptop
	myEngine->AddMediaFolder("D:\\Nhendley\\Documents\\GitHub\\Thief-Rewrite\\models");

	/**** Set up your scene here ****/

	//setting up character states
	guardState = idle;
	noiseLevel = quiet;
	movementLevel = walking;

	//Models & Meshes, setup!
	ICamera* camera = myEngine->CreateCamera(kManual, 0, 5, -10);
	IFont* myFont = myEngine->LoadFont("Calibri", 30);

	IMesh* guardMesh = myEngine->LoadMesh("casual_A.x");
	IModel* guard = guardMesh->CreateModel(0.0f, 0.0f, -20.0f);

	IMesh* stateMesh = myEngine->LoadMesh("state.x");
	IModel* state = stateMesh->CreateModel(0.0, 3.0, 0.0);
	state->AttachToParent(guard);

	IMesh* sierraMesh = myEngine->LoadMesh("sierra.x");
	IModel* sierra = sierraMesh->CreateModel(20.0, 0.0, -20.0);
	camera->AttachToParent(sierra);
	camera->RotateX(20.0f);

	IMesh* floorMesh = myEngine->LoadMesh("floor.x");
	IModel* floor = floorMesh->CreateModel(0, -0.01, 0);

	IMesh* dummyMesh = myEngine->LoadMesh("dummy.x");
	IModel* dummy[5];

	IMesh* noisyFloorMesh = myEngine->LoadMesh("square.x");
	IModel* noisyFloor[3];

	//couldn't think of an effective pattern for a for loop so eh!
	//waypoints
	dummy[0] = dummyMesh->CreateModel(20.0f, 0.0f, 0.0f);
	dummy[1] = dummyMesh->CreateModel(0.0f, 0.0f, 20.0f);
	dummy[2] = dummyMesh->CreateModel(-20.0f, 0.0f, 0.0f);
	dummy[3] = dummyMesh->CreateModel(0.0f, 0.0f, -20.0f);
	dummy[4] = dummyMesh->CreateModel(20.0f, 0.0f, -10.0f);

	//noisy floor panels
	noisyFloor[0] = noisyFloorMesh->CreateModel(0.0f, -5.0f, 20.0f);
	noisyFloor[1] = noisyFloorMesh->CreateModel(0.0f, -5.0f, -20.0f);
	noisyFloor[2] = noisyFloorMesh->CreateModel(20.0f, -5.0f, -10.0f);

	//sets the floor panels to be red
	for (int i = 0; i < 3; i++)
	{
		noisyFloor[i]->SetSkin("red.PNG");
		noisyFloor[i]->RotateX(90.0f);
	}

	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning())
	{
		// Draw the scene
		myEngine->DrawScene();
		gameSpeed = myEngine->Timer(); //used for multiplying movement speeds.

		//this bit handles the thief's movements and handling how noisy she is
		characterMovement(sierra, gameSpeed);
		floorHandler(sierra, noisyFloor);
		//

		//handling all the guard things - facing vector, distance to thief, dot product resolution
		guardFacingVector(guard, sierra);
		distanceToThief = guardToThiefDistance(guard, sierra);
		dotProductResult = dotProduct(guard, sierra);
		detectionHandler(distanceToThief, noiseLevel, dotProductResult, guardState);
		side = guardLeftorRight();
		//

		//handles guard actions
		if (guardState == idle)
		{
			guardPatrol(guard, dummy);
			state->SetSkin("blue.PNG");
		}
		if (guardState == alert)
		{
			Look(guard);	//Custom Look fnction is done	
			state->SetSkin("red.PNG");	//Slow-ish turn until cross product returns 0(?), then chase (musing)
			guard->MoveLocalZ(guardChasing * gameSpeed);
		}
		if (guardState == dead)
		{
			state->SetSkin("purple.PNG");
		}
		
		//Testing purposes
		textOutput(myFont, dotProductResult, distanceToThief, guardState, noiseLevel, movementLevel);
		
	}

	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}
//outputs variables to screen for sanity checking. 
void textOutput(IFont* font, float angle, float distance, enum Guard guardState, enum Thief thiefNoise, enum ThiefMovement movementType)
{
	string sideText;
	if (side < 0)
	{
		sideText = "Left";
	}
	else
	{
		sideText = "Right";
	}

	stringstream angleOutput;
	stringstream distanceOutput;
	stringstream stateOutput;
	stringstream noisyOutput;
	stringstream speedOutput;
	stringstream sideof;

	sideof << "Side: " << sideText;
	angleOutput << "Dot Product: " <<  angle;
	distanceOutput << "Distance to Guard: " << distance;
	stateOutput << "Guard Alert State: " << guardState;
	noisyOutput << "Noise: " << thiefNoise;
	speedOutput << "Movement Speed: " << movementType;

	font->Draw(angleOutput.str(), 850, 620, kBlue);
	font->Draw(distanceOutput.str(), 850, 640, kBlue);
	font->Draw(stateOutput.str(), 850, 660, kBlue);
	font->Draw(noisyOutput.str(), 850, 150, kBlue);
	font->Draw(speedOutput.str(), 850, 180, kBlue);
	font->Draw(sideof.str(), 850, 600, kBlue);
}

//Handles character movement and speed changing
void characterMovement(IModel* character, float frameTimer)
{
	//movement type setting
	if (myEngine->KeyHit(run))
	{
		movementLevel = running;
	}
	if (myEngine->KeyHit(walk))
	{
		movementLevel = walking;
	}
	if (myEngine->KeyHit(creep))
	{
		movementLevel = creeping;
	}

	//movement handling
	if (myEngine->KeyHeld(forwards))
	{
		switch (movementLevel)
		{
		case running:
			character->MoveLocalZ(thiefRun * frameTimer);
			break;
		case walking:
			character->MoveLocalZ(thiefWalk * frameTimer);
			break;
		case creeping:
			character->MoveLocalZ(thiefCreep * frameTimer);
			break;
		}
	}
	if (myEngine->KeyHeld(backwards))
	{
		switch (movementLevel)
		{
		case running:
			character->MoveLocalZ(-thiefRun * frameTimer);
			break;
		case walking:
			character->MoveLocalZ(-thiefWalk * frameTimer);
			break;
		case creeping:
			character->MoveLocalZ(-thiefCreep * frameTimer);
			break;
		}
	}
	if (myEngine->KeyHeld(turnLeft))
	{
		character->RotateLocalY(-turnSpeed * frameTimer);
	}
	if (myEngine->KeyHeld(turnRight))
	{
		character->RotateLocalY(turnSpeed * frameTimer);
	}
	if (myEngine->KeyHit(gameExit))
	{
		myEngine->Stop();
	}
}

//guard patrol route thingy
void guardPatrol(IModel* guard, IModel* waypoint[])
{
	guard->LookAt(waypoint[currentWaypoint]);
	guard->MoveLocalZ(guardPatrolling * gameSpeed);

	//"collision" for waypoints
	for (int i = 0; i < 4; i++)
	{
		float x = guard->GetX() - waypoint[currentWaypoint]->GetX();
		float z = guard->GetZ() - waypoint[currentWaypoint]->GetZ();

		float distance = sqrt(x * x + z * z);

		if (distance <= 0.05f)
		{
			if (currentWaypoint < 4)
			{
				currentWaypoint++;
			}
			else
			{
				currentWaypoint = 0;
			}
		}
	}
}

//works out distance between thief and guard
float guardToThiefDistance(IModel* guard, IModel* character)
{
	float x = guardPosition.x - thiefPosition.x;
	float z = guardPosition.z - thiefPosition.z;
	return sqrt(x * x + z * z);
}

//sets thief to noisy if stepping on red panels
void floorHandler(IModel* thief, IModel* square[])
{
	//Collision Time
	float minX, maxX, minZ, maxZ;
	const float SIZE = 5.0f;
	for (int i = 0; i < 3; i++)
	{
		minX = square[i]->GetX() - SIZE;
		maxX = square[i]->GetX() + SIZE;
		minZ = square[i]->GetZ() - SIZE;
		maxZ = square[i]->GetZ() + SIZE;

		if (((thief->GetX() <= maxX && thief->GetX() >= minX && thief->GetZ() <= maxZ && thief->GetZ() >= minZ) && movementLevel == walking) || movementLevel == running)
		{
			noiseLevel = noisy;
			return; //stops the function feeding through again -> on advice from Gareth!
		}
		else
		{
			noiseLevel = quiet;
		}
	}
}

//gets guard's facing vector & sets guard's current pos and thief's current pos
void guardFacingVector(IModel* guard, IModel* thief)
{
	float guardMatrix[16];
	float thiefMatrix[16];
	guard->GetMatrix(guardMatrix);
	thief->GetMatrix(thiefMatrix);

	facingVector.Set(&guardMatrix[8]);
	guardPosition.Set(&guardMatrix[12]);
	thiefPosition.Set(&thiefMatrix[12]);
}

//holy heck it works now - gets dot product between guard and thief
//includes working out vector between the guard and thief
float dotProduct(IModel* guard, IModel* thief)
{
	//gets the vector between the thief and the guard
	vectorToThief = Subtract(thiefPosition, guardPosition);
	//Dot() uses these 2 to get the Dot Product. 
	return Dot(vectorToThief, facingVector);
}

void detectionHandler(float distance, enum Thief noise, float dotProduct, enum Guard &guardState)
{
	if ( ((dotProduct > 0.0f && distance < 8.0f) || (distance < 8.0f && noise == noisy)) && guardState != dead )
	{
		guardState = alert;
	}
	else if (guardState == alert && distance > 12.0f && guardState != dead)
	{
		guardState = idle;
	}
	else if (guardState == idle && distance < 1.5f && guardState != dead)
	{
		guardState = dead;
	}
}

float guardLeftorRight()
{
	CVector3 guardLocalX = Cross(kYAxis, facingVector);
	float side = Dot(vectorToThief, Cross(kYAxis,facingVector));
	return side;
}

void Look(IModel* guard)
{
	// Calculate matrix axes for guard
		// Get facing (z) vector from positions
	CVector3 vecZ = Normalise(Subtract(thiefPosition, guardPosition));
	// Use cross products to get other axes
	// Must normalise axes
	CVector3 vecX = Normalise(Cross(kYAxis, vecZ));
	CVector3 vecY = Normalise(Cross(vecZ, vecX));

	// Build matrix from axes + position
	// Matrix constructor using four CVector3 variables
	// - one for each row/column
	// (see matrix header)
	// Build matrix by row (axes + position)
	CMatrix4x4 guardMat;
	guardMat.MakeIdentity();
	guardMat.SetRow(0, vecX);
	guardMat.SetRow(1, vecY);
	guardMat.SetRow(2, vecZ);
	guardMat.SetRow(3, guardPosition);

	// Set position of guard using matrix
	guard->SetMatrix(&guardMat.e00);

}