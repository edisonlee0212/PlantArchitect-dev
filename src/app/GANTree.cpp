// PlantFactory.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//
#include <Application.hpp>

#ifdef RAYTRACERFACILITY
#include <CUDAModule.hpp>
#include <RayTracerManager.hpp>
#include "MLVQRenderer.hpp"
#endif

#include <EditorManager.hpp>
#include <Utilities.hpp>
#include <ProjectManager.hpp>
#include <PhysicsLayer.hpp>
#include <PostProcessing.hpp>

#include <CubeVolume.hpp>
#include <ClassRegistry.hpp>
#include <ObjectRotator.hpp>
#include "GeneralTreeBehaviour.hpp"
#include "DefaultInternodeResource.hpp"
#include "Internode.hpp"
#include <SpaceColonizationBehaviour.hpp>
#include "EmptyInternodeResource.hpp"
#include "LSystemBehaviour.hpp"
#include "AutoTreeGenerationPipeline.hpp"

#include "DefaultInternodePhyllotaxis.hpp"
#include "InternodeFoliage.hpp"
#include "RadialBoundingVolume.hpp"
#include "DepthCamera.hpp"
#include "MultipleAngleCapture.hpp"
#include "GANTreePipelineDriver.hpp"

#include "InternodeLayer.hpp"

using namespace PlantArchitect;
#ifdef RAYTRACERFACILITY
using namespace RayTracerFacility;
#endif
using namespace Scripts;

void EngineSetup();

int main() {
    ClassRegistry::RegisterPrivateComponent<ObjectRotator>("ObjectRotator");
    ClassRegistry::RegisterPrivateComponent<DepthCamera>("DepthCamera");
    ClassRegistry::RegisterPrivateComponent<AutoTreeGenerationPipeline>("AutoTreeGenerationPipeline");
    ClassRegistry::RegisterPrivateComponent<GANTreePipelineDriver>("GANTreePipelineDriver");
    ClassRegistry::RegisterAsset<MultipleAngleCapture>("MultipleAngleCapture", ".mulanglecap");

    EngineSetup();
    ApplicationConfigs applicationConfigs;
    applicationConfigs.m_projectPath = "GANTree/GANTree.ueproj";
    Application::Create(applicationConfigs);
#ifdef RAYTRACERFACILITY
    Application::PushLayer<RayTracerManager>();
#endif
    auto internodesLayer = Application::PushLayer<InternodeLayer>();
#pragma region Engine Loop
    Application::Start();
#pragma endregion
    Application::End();
}

void EngineSetup() {
    ProjectManager::SetScenePostLoadActions([=]() {
#pragma region Engine Setup
        Transform transform;
        transform.SetEulerRotation(glm::radians(glm::vec3(150, 30, 0)));
#pragma region Preparations
        Application::Time().SetTimeStep(0.016f);
        transform = Transform();
        transform.SetPosition(glm::vec3(0, 2, 35));
        transform.SetEulerRotation(glm::radians(glm::vec3(15, 0, 0)));
        auto mainCamera = EntityManager::GetCurrentScene()->m_mainCamera.Get<UniEngine::Camera>();
        if (mainCamera) {
            auto postProcessing =
                    mainCamera->GetOwner().GetOrSetPrivateComponent<PostProcessing>().lock();
            auto ssao = postProcessing->GetLayer<SSAO>().lock();
            ssao->m_kernelRadius = 0.1;
            mainCamera->GetOwner().SetDataComponent(transform);
            mainCamera->m_useClearColor = true;
            mainCamera->m_clearColor = glm::vec3(0.5f);
        }
#pragma endregion
#pragma endregion

        /*
         * Add all internode behaviours for example.
         */
        auto internodesLayer = Application::GetLayer<InternodeLayer>();
        auto spaceColonizationBehaviour = internodesLayer->GetInternodeBehaviour<SpaceColonizationBehaviour>();
        Entity cubeVolumeEntity = EntityManager::CreateEntity(EntityManager::GetCurrentScene(), "CubeVolume");
        Transform cubeVolumeTransform = cubeVolumeEntity.GetDataComponent<Transform>();
        cubeVolumeTransform.SetPosition(glm::vec3(0, 12.5, 0));
        cubeVolumeEntity.SetDataComponent(cubeVolumeTransform);

        auto cubeVolume = cubeVolumeEntity.GetOrSetPrivateComponent<CubeVolume>().lock();
        cubeVolume->m_minMaxBound.m_min = glm::vec3(-10.0f);
        cubeVolume->m_minMaxBound.m_max = glm::vec3(10.0f);
        spaceColonizationBehaviour->PushVolume(std::dynamic_pointer_cast<IVolume>(cubeVolume));
        /*
         * Add all pipelines
         */
        auto multipleAngleCapturePipelineEntity = EntityManager::CreateEntity(EntityManager::GetCurrentScene(),
                                                                              "GANTree Dataset Pipeline");
        auto multipleAngleCapturePipeline = multipleAngleCapturePipelineEntity.GetOrSetPrivateComponent<AutoTreeGenerationPipeline>().lock();
        auto multipleAngleCapture = AssetManager::CreateAsset<MultipleAngleCapture>();
        multipleAngleCapture->m_cameraEntity = mainCamera->GetOwner();
        multipleAngleCapturePipeline->m_pipelineBehaviour = multipleAngleCapture;
        multipleAngleCapture->m_volume = cubeVolume;

        auto driver = multipleAngleCapturePipelineEntity.GetOrSetPrivateComponent<GANTreePipelineDriver>().lock();
        driver->m_pipeline = multipleAngleCapturePipeline;
    });
}