#pragma once

#include "InternodeRingSegment.hpp"
#include <plant_architect_export.h>
#include <IInternodeResource.hpp>

using namespace UniEngine;
namespace PlantArchitect {
    class InternodeSystem;
    struct LString;
    enum PLANT_ARCHITECT_API class BudType {
        Apical,
        Lateral
    };
    struct PLANT_ARCHITECT_API Bud : public ISerializable {
        BudType m_type;
        bool m_dormant;
        float m_branchingAngle;
        float m_rollAngle;
    };
    struct LSystemCommand;

    class PLANT_ARCHITECT_API Internode : public IPrivateComponent {
        std::weak_ptr<InternodeSystem> m_internodeSystem;

        void ExportLSystemCommandsHelper(const Entity &target, std::vector<LSystemCommand> &commands);

        void CollectInternodesHelper(const Entity &target, std::vector<Entity> &results);

    public:
        void CollectInternodes(std::vector<Entity> &results);

        AssetRef m_skinnedBranchMesh;
        AssetRef m_skinnedFoliageMesh;
        glm::vec3 m_normalDir = glm::vec3(0, 0, 1);
        bool m_meshGenerated = false;
        bool m_foliageGenerated = false;
        int m_step;
        std::vector<InternodeRingSegment> m_rings;
        /**
         * The resource storage for the internode.
         */
        std::unique_ptr<IInternodeResource> m_resource;
        /**
         * The apical or terminal bud.
         */
        Bud m_apicalBud;
        /**
         * The axillary or lateral bud.
         */
        std::vector<Bud> m_lateralBuds;

        /**
         * Actions to take when the internode is retrieved from the factory.
         */
        void OnRetrieve();

        /**
         * Action to take when the internode is recycled to the factory.
         */
        void OnRecycle();

        /**
         * Collect resource (auxin, nutrients, etc.)
         * @param deltaTime How much time the action takes.
         */
        void CollectResource(float deltaTime);

        /**
         * Down stream the resources.
         * @param deltaTime
         */
        void DownStreamResource(float deltaTime);

        /**
         * Up stream the resources.
         * @param deltaTime How much time the action takes.
         */
        void UpStreamResource(float deltaTime);

        void OnCreate() override;

        void Clone(const std::shared_ptr<IPrivateComponent> &target);
        /*
         * Parse the structure of the internodes and set up commands.
         */
        void ExportLString(const std::shared_ptr<LString>& lString);

        void OnInspect() override;
    };

}