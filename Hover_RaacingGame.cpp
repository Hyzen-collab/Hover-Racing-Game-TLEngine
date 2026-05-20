// Hover_RaacingGame.cpp: A program using the TL-Engine
//Student Name: K.A.I.Piumika
//Student ID: G21328023

#include <TL-Engine.h>  // TL-Engine include file and namespace
#include <vector>       // STL Vector for objects
#include <string>       
#include <cmath>        // Math functions
#include <chrono>       // For sleep functionality
#include <thread>       // For sleep functionality

using namespace tle;
using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const float PI = 3.14f; //Move accurate PI

const float unitToMeters = 2.0f;  //TL Engine Unit = 2m
const float metersToKMH = 3.6f;   //1ms = 3.6 km/h

const float mouseSensitivity = 0.1f;
const float maxPitch = 60.0f; // Prevents camera flipping

// Structure for 2D Vector calculations
struct HoverVector
{
    float x, z;

    HoverVector operator+(const HoverVector& other) const { return { x + other.x, z + other.z }; }
    HoverVector operator*(float scalar) const { return { x * scalar, z * scalar }; }

    float Length() const { return sqrt(x * x + z * z); }

    void Normalization()
    {
        float length = Length();
        if (length != 0) {
            x /= length;
            z /= length;
        }
    }
};

// Structure to store game objects
struct SceneObject
{
    IModel* model;
    string objecttype;
    float posX, posY, posZ;
    float rotation;
};

vector<SceneObject> checkpoints, walls, isles, tanks; //Containers(vectors) for game objectss

//Collision Detection (Sphere to Box Collision for walls)
bool SpToBoxCollision(HoverVector spherePos, SceneObject& box, float radius)
{
    float halfSize = 5.0f; // Adjust based on actual wall size
    float closestX = max(box.posX - halfSize, min(spherePos.x, box.posX + halfSize));
    float closestZ = max(box.posZ - halfSize, min(spherePos.z, box.posZ + halfSize));

    float distanceX = spherePos.x - closestX;
    float distanceZ = spherePos.z - closestZ;

    return (distanceX * distanceX + distanceZ * distanceZ) < (radius * radius);
}

// Sphere to Sphere Collision (for tanks)
bool SpToSphereCollision(HoverVector sphere1Pos, SceneObject& sphere2, float radius)
{
    float dx = sphere1Pos.x - sphere2.posX;
    float dz = sphere1Pos.z - sphere2.posZ;

    return (dx * dx + dz * dz) < (radius * radius);
}

// Point to Box Collision (for checkpoints)
bool PointToBoxCollision(HoverVector point, SceneObject& box, float boxWidth, float boxDepth)
{
    return (point.x > box.posX - boxWidth / 2 && point.x < box.posX + boxWidth / 2 &&
        point.z > box.posZ - boxDepth / 2 && point.z < box.posZ + boxDepth / 2);
}

// Function to Create Scene Models
void CreateModels(I3DEngine* myEngine)
{
    // Load meshes
    IMesh* skyMesh = myEngine->LoadMesh("Skybox 07.x");
    IMesh* groundMesh = myEngine->LoadMesh("ground.x");
    IMesh* checkpointMesh = myEngine->LoadMesh("Checkpoint.x");
    IMesh* wallMesh = myEngine->LoadMesh("Wall.x");
    IMesh* isleMesh = myEngine->LoadMesh("IsleStraight.x");
    IMesh* tankMesh = myEngine->LoadMesh("TankSmall1.x");

    // Create Skybox and Ground
    skyMesh->CreateModel(0, -840, 0);
    groundMesh->CreateModel(0, 0, 0);

    // Checkpoints
    float checkpointPositions[3][4] = { {0, 0, 0, 0}, {10, 0, 120, 90}, {25, 0, 56, 0} };
    for (auto& pos : checkpointPositions)
    {
        SceneObject obj = { checkpointMesh->CreateModel(pos[0], pos[1], pos[2]), "Checkpoint", pos[0], pos[1], pos[2], pos[3] };
        obj.model->RotateY(pos[3]);
        checkpoints.push_back(obj);
    }

    // Walls
    float wallPositions[4][4] = { {-10, 0, 48, 0}, {10, 0, 48, 0}, {-10, 0, 64, 0}, {10, 0, 64, 0} };
    for (auto& pos : wallPositions)
    {
        walls.push_back({ wallMesh->CreateModel(pos[0], pos[1], pos[2]), "Wall", pos[0], pos[1], pos[2], pos[3] });
    }

    // Isles
    float islePositions[6][4] = { {-10, 0, 40, 0}, {10, 0, 40, 0}, {-10, 0, 56, 0}, {10, 0, 56, 0}, {-10, 0, 72, 0}, {10, 0, 72, 0} };
    for (auto& pos : islePositions)
    {
        isles.push_back({ isleMesh->CreateModel(pos[0], pos[1], pos[2]), "Isle", pos[0], pos[1], pos[2], pos[3] });
    }

    // Tanks (Obstacles)
    float tankPositions[2][4] = { {-5, 0, 90, 0}, {15, 0, 30, 0} };
    for (auto& pos : tankPositions)
    {
        tanks.push_back({ tankMesh->CreateModel(pos[0], pos[1], pos[2]), "Tank", pos[0], pos[1], pos[2], pos[3] });
    }
}

void main()
{
    // Create a 3D engine (using TLX engine here) and open a window for it
    I3DEngine* myEngine = New3DEngine(kTLX);
    myEngine->StartWindowed();

    // Add default folder for meshes and other media
    myEngine->AddMediaFolder(".\\Media");
    CreateModels(myEngine);

    // Load Hover Bike
    IMesh* hoverMesh = myEngine->LoadMesh("race2.x");
    IModel* hover = hoverMesh->CreateModel(0, 0, -20);
    float carRotation = 0.0f;
    HoverVector speedVector = { 0, 0 };

    IFont* myFont = myEngine->LoadFont("Arial", 36);

    //Car Attributes
    float acceleration = 200.0f;
    float dragResistance = 0.98f;
    float rotationSpeed = 100.0f;

    // Camera
    ICamera* camera = myEngine->CreateCamera(kManual);
    camera->SetPosition(0, 10, -30);
    camera->AttachToParent(hover);
    float cameraOffsetZ = -20.0f;
    float cameraOffsetY = 10.0f;

    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;


    // Game States
    string dialog = "Press Space to Start";
    bool gameStarted = false;
    int currentCheckpoint = 0;

    // The main game loop, repeat until engine is stopped
    while (myEngine->IsRunning())
    {

        // Draw the scene
        myEngine->DrawScene();

        /**** Update your scene each frame here ****/

        float frameTime = myEngine->Timer();

        // Mouse Camera Control
        float mouseX = myEngine->GetMouseMovementX();
        float mouseY = myEngine->GetMouseMovementY();

        cameraYaw += mouseX * mouseSensitivity;
        cameraPitch -= mouseY * mouseSensitivity; // Inverted to feel natural

        // Clamp pitch to prevent flipping
        if (cameraPitch > maxPitch) cameraPitch = maxPitch;
        if (cameraPitch < -maxPitch) cameraPitch = -maxPitch;


        // Apply rotations
        camera->RotateY(mouseX * mouseSensitivity);
        camera->RotateLocalX(-mouseY * mouseSensitivity); // Negative for natural inversion

        // Reset camera with '1'
        if (myEngine->KeyHit(Key_1))
        {
            camera->SetPosition(hover->GetX(), hover->GetY() + cameraOffsetY, hover->GetZ() + cameraOffsetZ);
            camera->LookAt(hover);
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
        }

        HoverVector newPosition = { hover->GetX() + speedVector.x * frameTime, hover->GetZ() + speedVector.z * frameTime };
        bool collision = false;

        // Checking collision with walls
        for (auto& wall : walls)
        {
            if (SpToBoxCollision(newPosition, wall, 2.0f))
            {
                collision = true;
                break;
            }
        }

        // Checking collision with isles
        for (auto& isle : isles)
        {
            if (SpToBoxCollision(newPosition, isle, 2.0f))
            {
                collision = true;
                break;
            }
        }

        // Checking collision with tanks
        for (auto& tank : tanks)
        {
            if (SpToSphereCollision(newPosition, tank, 3.0f))
            {
                collision = true;
                break;
            }
        }

        if (!collision)
        {
            hover->Move(speedVector.x * frameTime, 0, speedVector.z * frameTime);
        }
        else {
            speedVector = { 0, 0 };
        }

        if (!gameStarted)
        {
            if (myEngine->KeyHit(Key_Space))
            {
                dialog = "3"; myFont->Draw(dialog, 20, 20, kWhite);
                myEngine->DrawScene();  //Manually update the scene for each countdown number (Ensure text updates are visible before waiting)
                sleep_for(seconds(1));

                dialog = "2"; myFont->Draw(dialog, 20, 20, kWhite);
                myEngine->DrawScene();
                sleep_for(seconds(1));

                dialog = "1"; myFont->Draw(dialog, 20, 20, kWhite);
                myEngine->DrawScene();
                sleep_for(seconds(1));

                dialog = "Go!";
                gameStarted = true;
            }
            myFont->Draw(dialog, 20, 20, kWhite);
            myEngine->DrawScene(); // Ensure the lastest text is drawn
            continue;
        }


        // Hover Car Movement
       // if (myEngine->KeyHeld(Key_W)) car->MoveLocalZ(thrustForce * frameTime);
       //if (myEngine->KeyHeld(Key_S)) car->MoveLocalZ(-thrustForce * 0.5f * frameTime);

        if (myEngine->KeyHeld(Key_W))
            speedVector = speedVector + HoverVector{ sin(carRotation * PI / 189.0f), cos(carRotation * PI / 180.0f) } *acceleration * frameTime;

        if (myEngine->KeyHeld(Key_S))
            speedVector = speedVector + HoverVector{ sin(carRotation * PI / 180.0f), cos(carRotation * PI / 180.0f) } *(-acceleration) * frameTime;

        speedVector = speedVector * dragResistance;  //apply drag

        if (myEngine->KeyHeld(Key_A))
        {
            carRotation -= rotationSpeed * frameTime;
            hover->RotateY(-rotationSpeed * frameTime);
        }

        if (myEngine->KeyHeld(Key_D))
        {
            carRotation += rotationSpeed * frameTime;
            hover->RotateY(rotationSpeed * frameTime);
        }

        //Speed  Calculation and display
        ////////////////////////////////
        float speed = speedVector.Length() * unitToMeters * metersToKMH;
        myFont->Draw("Speed: " + to_string((int)speed) + "kmh", 20, 60, kWhite);

        // Camera Controls
        if (myEngine->KeyHeld(Key_Up)) camera->MoveZ(5.0f * frameTime);
        if (myEngine->KeyHeld(Key_Down)) camera->MoveZ(-5.0f * frameTime);
        if (myEngine->KeyHeld(Key_Left)) camera->MoveX(-5.0f * frameTime);
        if (myEngine->KeyHeld(Key_Right)) camera->MoveX(5.0f * frameTime);
        //if (myEngine->KeyHit(Key_1)) camera->SetPosition(hover->GetX(), 10, hover->GetZ() - 30);

        //Checkpoint Collision
        HoverVector carPosition = { hover->GetX(), hover->GetZ() };
        if (currentCheckpoint < checkpoints.size() &&
            PointToBoxCollision(carPosition, checkpoints[currentCheckpoint], 10.0f, 10.0f))
        {
            dialog = "Stage " + to_string(currentCheckpoint + 1) + " Complete";
            currentCheckpoint++;

            if (currentCheckpoint == checkpoints.size())
            {
                dialog = "Race Complete";
            }
        }

        //camera->SetPosition(car->GetX(), car->GetY() + cameraOffsetY, car->GetZ() + cameraOffsetZ);
       // camera->LookAt(car);

        myFont->Draw(dialog, 20, 20, kWhite);


    }

    // Delete the 3D engine now we are finished with it
    myEngine->Delete();
}




