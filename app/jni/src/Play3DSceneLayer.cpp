#include "Play3DSceneLayer.h"

Play3DSceneLayer::Play3DSceneLayer(std::shared_ptr<PlayGUILayer> guiLayer)
{
    m_ID = Beryll::LayerID::PLAY_SCENE;

    m_guiLayer = std::move(guiLayer);

    simpleCubeSphere = std::make_shared<Beryll::SimpleObject>("models/cube_text.dae", false, "shaders/GLES/SimpleModel.vert", "shaders/GLES/SimpleModel.frag", "diffuseTexture");
    m_gameObjects.push_back(simpleCubeSphere);
    enemyMan = std::make_shared<Beryll::AnimatedObject>("models/model.dae",
                                                        true,
                                                        "shaders/GLES/AnimatedModel.vert",
                                                        "shaders/GLES/AnimatedModel.frag",
                                                        "diffuseTexture");
    m_gameObjects.push_back(enemyMan);
    //worm = std::make_shared<Beryll::CollidingAnimatedObject>("models/collisionsAnim.dae", false, "shaders/GLES/AnimatedModel.vert", "shaders/GLES/AnimatedModel.frag", "diffuseTexture");
    //m_gameObjects.push_back(worm);
    collPlane = std::make_shared<Beryll::CollidingSimpleObject>("models/CollisionStaticGround.dae",
                                                                false,
                                                                0,
                                                                false,
                                                                Beryll::CollisionFlags::STATIC,
                                                                Beryll::CollisionGroups::GROUND,
                                                                Beryll::CollisionGroups::PLAYER | Beryll::CollisionGroups::CUBE | Beryll::CollisionGroups::CAMERA,
                                                                "shaders/GLES/SimpleModel.vert",
                                                                "shaders/GLES/SimpleModel.frag",
                                                                "diffuseTexture");
    m_gameObjects.push_back(collPlane);

    collWall = std::make_shared<Beryll::CollidingSimpleObject>("models/wall.dae",
                                                                false,
                                                                0,
                                                                true,
                                                                Beryll::CollisionFlags::STATIC,
                                                                Beryll::CollisionGroups::WALL,
                                                                Beryll::CollisionGroups::PLAYER | Beryll::CollisionGroups::CUBE | Beryll::CollisionGroups::CAMERA,
                                                                "shaders/GLES/SimpleModel.vert",
                                                                "shaders/GLES/SimpleModel.frag",
                                                                "diffuseTexture");
    m_gameObjects.push_back(collWall);

    // player
    player = std::make_shared<Beryll::CollidingSimpleObject>("models/CollisionCube.dae",
                                                             false,
                                                             0,
                                                             true,
                                                             Beryll::CollisionFlags::KINEMATIC,
                                                             Beryll::CollisionGroups::PLAYER,
                                                             Beryll::CollisionGroups::ALL_GROUPS,
                                                             "shaders/GLES/SimpleModel.vert",
                                                             "shaders/GLES/SimpleModel.frag",
                                                             "diffuseTexture");
    m_gameObjects.push_back(player);

    for(int i = 0; i < 10; ++i)
    {
        collWarms.push_back(std::make_shared<Beryll::CollidingAnimatedObject>("models/collisionsAnim.dae",
                                                                              false,
                                                                              15,
                                                                              false,
                                                                              Beryll::CollisionFlags::DYNAMIC,
                                                                              Beryll::CollisionGroups::CUBE,
                                                                              Beryll::CollisionGroups::GROUND | Beryll::CollisionGroups::WALL |
                                                                              Beryll::CollisionGroups::CUBE | Beryll::CollisionGroups::PLAYER,
                                                                              "shaders/GLES/AnimatedModel.vert",
                                                                              "shaders/GLES/AnimatedModel.frag",
                                                                              "diffuseTexture"));
        m_gameObjects.push_back(collWarms.back());
    }

    Beryll::Physics::setAngularFactor(player->getID(), glm::vec3(0.0f));
    Beryll::Physics::disableGravityForObject(player->getID());

    Beryll::Camera::setCameraPos(player->getPosition() + m_cameraOffset);
    Beryll::Camera::setCameraFront(player->getPosition());
}

Play3DSceneLayer::~Play3DSceneLayer()
{

}

void Play3DSceneLayer::updateBeforePhysics()
{
    bool cameraSeeObject = false;
    for(const std::shared_ptr<Beryll::GameObject>& go : m_gameObjects)
    {
        // disable objects for performance
/*
        cameraSeeObject = Beryll::Camera::getIsSeeObject(go->getPosition());

        if(go->getIsEnabled() && !cameraSeeObject)
        {
            go->disable();
            if(go->getHasCollisionObject() && go->getCanBeDisabled())
                Beryll::Physics::softRemoveObject(go->getID());
        }
        else if(!go->getIsEnabled() && cameraSeeObject)
        {
            go->enable();
            if(go->getHasCollisionObject() && go->getCanBeDisabled())
                Beryll::Physics::restoreObject(go->getID());
        }
*/
        // 1. let objects update themselves
        if(go->getIsEnabled())
        {
            go->updateBeforePhysics();
        }
    }

    std::vector<Beryll::Finger> fingers = Beryll::EventHandler::getFingers();

    for(const Beryll::Finger& f : fingers)
    {
        if(f.handled) { continue; }

        if(f.downEvent)
        {
            m_lastFingerMovePosX = f.SDL2ScreenPos.x;
            m_lastFingerMovePosY = f.SDL2ScreenPos.y;
            //enemyWorm->setAnimation("Armature|Cube1_jump");
            break;
        }
        else
        {
            float deltaX = f.SDL2ScreenPos.x - m_lastFingerMovePosX;
            float deltaY = f.SDL2ScreenPos.y - m_lastFingerMovePosY;

            // euler angles * distance
            m_cameraOffset.x = (glm::cos(glm::radians(m_angleXZ)) * glm::cos(glm::radians(m_angleYZ)))  * 100;
            m_cameraOffset.y = glm::sin(glm::radians(m_angleYZ))  * 100;
            m_cameraOffset.z = (glm::sin(glm::radians(m_angleXZ)) * glm::cos(glm::radians(m_angleYZ)))  * 100;

            m_angleXZ += deltaX;
            m_angleYZ += deltaY;
            if(m_angleYZ < -89.0f) m_angleYZ = -89.0f;
            if(m_angleYZ > 89.0f) m_angleYZ = 89.0f;
            m_lastFingerMovePosX = f.SDL2ScreenPos.x;
            m_lastFingerMovePosY = f.SDL2ScreenPos.y;
            break;
        }
    }

    // 2. update them manualy based on GUI layer buttons state
    if(m_guiLayer->buttonMove->getIsPressed())
    {
        glm::vec3 newCubePos = glm::vec3(player->getPosition().x + Beryll::Camera::getCameraDirection().x,
                                         player->getPosition().y + Beryll::Camera::getCameraDirection().y,
                                         player->getPosition().z + Beryll::Camera::getCameraDirection().z);
        player->setPosition(newCubePos);
        Beryll::Physics::setTransforms(player->getID(), newCubePos, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), false);
        //BR_INFO("newCubePos x:{0} y:{1} z:{2}", newCubePos.x, newCubePos.y, newCubePos.z);
        //BR_INFO("Camera Direction x:{0} y:{1} z:{2}", Beryll::Camera::getCameraDirection().x, Beryll::Camera::getCameraDirection().y, Beryll::Camera::getCameraDirection().z);
    }

    // 3. pass final position to Physics module
    if(m_guiLayer->buttonResetCube->getIsPressed())
    {
        for(const auto& warm : collWarms)
        {
            int randX = Beryll::RandomGenerator::getFastInt(0, 200);
            int randZ = Beryll::RandomGenerator::getFastInt(0, 200) - 100;
            warm->setPosition(glm::vec3(randX, 100, randZ));
            Beryll::Physics::setTransforms(warm->getID(), glm::vec3(randX, 100, randZ));
        }
    }
}

void Play3DSceneLayer::updateAfterPhysics()
{
    // 1. let objects update themselves based on physics simulation
    for(const std::shared_ptr<Beryll::GameObject>& go : m_gameObjects)
    {
        if(go->getIsEnabled())
        {
            go->updateAfterPhysics();
        }
    }

    // 2. resolve collisions
    //if(Beryll::Physics::getIsCollision(collWall->getID(), player->getID()))
    //    BR_INFO("Player collising with wall");
    //if(Beryll::Physics::getIsCollision(collPlane->getID(), player->getID()))
    //   BR_INFO("Player collising with ground");

    // 3. use objects updated position
    Beryll::RayClosestHit rayClosestHit = Beryll::Physics::castRayClosestHit(player->getPosition(),
                                                                          player->getPosition() + m_cameraOffset,
                                                                          Beryll::CollisionGroups::CAMERA,
                                                                          Beryll::CollisionGroups::GROUND | Beryll::CollisionGroups::WALL);
    if(rayClosestHit)
        Beryll::Camera::setCameraPos(rayClosestHit.hitPoint);
    else
        Beryll::Camera::setCameraPos(player->getPosition() + m_cameraOffset);

    Beryll::Camera::setCameraFront(player->getPosition());
}

void Play3DSceneLayer::draw()
{
    for(const std::shared_ptr<Beryll::GameObject>& go : m_gameObjects)
    {
        if(go->getIsEnabled())
        {
            go->draw();
        }
    }
}

void Play3DSceneLayer::playSound()
{
    for(const std::shared_ptr<Beryll::GameObject>& go : m_gameObjects)
    {
        if(go->getIsEnabled())
        {
            go->playSound();
        }
    }
}