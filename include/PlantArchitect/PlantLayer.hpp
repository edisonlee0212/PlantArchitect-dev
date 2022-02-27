#pragma once
#include "Application.hpp"
#include "plant_architect_export.h"
#include <VoxelSpace.hpp>
#include "PlantDataComponents.hpp"
#include "ILayer.hpp"
#include "FBM.hpp"
using namespace UniEngine;
namespace PlantArchitect {
    enum class BranchColorMode{
        None,
        Order,
        Level,
        Water,
        ApicalControl,
        WaterPressure,
        Proximity,
        Inhibitor,
        IndexDivider,
        IndexRange,
        StrahlerNumber,
    };
    struct BranchPhysicsParameters;
    class IPlantBehaviour;
    class PLANT_ARCHITECT_API PlantLayer : public ILayer {
        void PreparePhysics(const Entity& entity, const Entity& child, const BranchPhysicsParameters& branchPhysicsParameters);

    public:
        FBM m_fBMField;
        float m_forceFactor = 1.0f;
        bool m_applyFBMField = false;
        bool m_enablePhysics = false;
        void DrawColorModeSelectionMenu();
        void PreparePhysics();
        void CalculateStatistics();
        /**
         * The EntityQuery for filtering all internodes.
         */
        EntityQuery m_internodesQuery;
        EntityQuery m_branchesQuery;
        void FixedUpdate() override;
        void LateUpdate() override;

        void Simulate(int iterations);

        void OnCreate() override;

        void OnInspect() override;

        std::shared_ptr<Camera> m_visualizationCamera;

        template<typename T>
        void PushInternodeBehaviour(const std::shared_ptr<T>& behaviour);
        template<typename T>
        std::shared_ptr<T> GetInternodeBehaviour();
        /**
         * Check if the entity is valid internode.
         * @param target Target for check.
         * @return True if the entity is valid and contains [InternodeInfo] and [Internode], false otherwise.
         */
        static bool InternodeCheck(const Entity& target);

        BranchColorMode m_branchColorMode = BranchColorMode::None;
        int m_indexDivider = 512;
        int m_indexRangeMin = 128;
        int m_indexRangeMax = 512;
        void UpdateInternodeColors();
        void UpdateInternodeCylinder();
        void UpdateBranchColors();
        void UpdateBranchCylinder();
        void UpdateInternodePointer(const float &length,
                                    const float &width = 0.01f);
    private:
        VoxelSpace m_voxelSpace;

        friend class Internode;

        std::vector<AssetRef> m_internodeBehaviours;


#pragma region Internode debugging camera

        int m_visualizationCameraResolutionX = 1;
        int m_visualizationCameraResolutionY = 1;
        float m_lastX = 0;
        float m_lastY = 0;
        float m_lastScrollY = 0;
        bool m_startMouse = false;
        bool m_startScroll = false;
        bool m_rightMouseButtonHold = false;
        EntityRef m_currentFocusingInternode = Entity();

#pragma region Rendering

        float m_pointerLength = 0.4f;
        float m_pointerWidth = 0.02f;

        bool m_drawInternodes = true;
        bool m_drawBranches = true;
        bool m_drawPointers = false;

        float m_internodeTransparency = 0.5f;
        float m_branchTransparency = 0.5f;

        bool m_autoUpdate = true;

        float m_internodeColorValueMultiplier = 1.0f;
        float m_internodeColorValueCompressFactor = 0.0f;
        glm::vec3 m_branchColor = glm::vec3(1, 1, 0);
        glm::vec3 m_internodeColor = glm::vec3(0, 1, 0);
        std::vector<glm::vec3> m_randomColors;
        glm::vec4 m_pointerColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

        std::vector<Entity> m_entitiesWithRenderer;
        OpenGLUtils::GLVBO m_internodeColorBuffer;

        void UpdateInternodeCamera();
        void RenderInternodeCylinders();
        void RenderInternodePointers();
        void RenderBranchCylinders();
#pragma endregion
#pragma endregion
    };

}