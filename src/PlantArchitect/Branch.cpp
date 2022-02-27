//
// Created by lllll on 2/26/2022.
//

#include "Branch.hpp"
using namespace PlantArchitect;

void BranchPhysicsParameters::Serialize(YAML::Emitter &out) {
    out << YAML::Key << "m_density" << YAML::Value << m_density;
    out << YAML::Key << "m_linearDamping" << YAML::Value << m_linearDamping;
    out << YAML::Key << "m_angularDamping" << YAML::Value << m_angularDamping;
    out << YAML::Key << "m_positionSolverIteration" << YAML::Value << m_positionSolverIteration;
    out << YAML::Key << "m_velocitySolverIteration" << YAML::Value << m_velocitySolverIteration;
    out << YAML::Key << "m_jointDriveStiffnessFactor" << YAML::Value << m_jointDriveStiffnessFactor;
    out << YAML::Key << "m_jointDriveStiffnessThicknessFactor" << YAML::Value << m_jointDriveStiffnessThicknessFactor;
    out << YAML::Key << "m_jointDriveDampingFactor" << YAML::Value << m_jointDriveDampingFactor;
    out << YAML::Key << "m_jointDriveDampingThicknessFactor" << YAML::Value << m_jointDriveDampingThicknessFactor;
    out << YAML::Key << "m_enableAccelerationForDrive" << YAML::Value << m_enableAccelerationForDrive;
}

void BranchPhysicsParameters::Deserialize(const YAML::Node &in) {
    if (in["m_density"]) m_density = in["m_density"].as<float>();
    if (in["m_linearDamping"]) m_linearDamping = in["m_linearDamping"].as<float>();
    if (in["m_angularDamping"]) m_angularDamping = in["m_angularDamping"].as<float>();
    if (in["m_positionSolverIteration"]) m_positionSolverIteration = in["m_positionSolverIteration"].as<int>();
    if (in["m_velocitySolverIteration"]) m_velocitySolverIteration = in["m_velocitySolverIteration"].as<int>();
    if (in["m_jointDriveStiffnessFactor"]) m_jointDriveStiffnessFactor = in["m_jointDriveStiffnessFactor"].as<float>();
    if (in["m_jointDriveStiffnessThicknessFactor"]) m_jointDriveStiffnessThicknessFactor = in["m_jointDriveStiffnessThicknessFactor"].as<float>();
    if (in["m_jointDriveDampingFactor"]) m_jointDriveDampingFactor = in["m_jointDriveDampingFactor"].as<float>();
    if (in["m_jointDriveDampingThicknessFactor"]) m_jointDriveDampingThicknessFactor = in["m_jointDriveDampingThicknessFactor"].as<float>();
    if (in["m_enableAccelerationForDrive"]) m_enableAccelerationForDrive = in["m_enableAccelerationForDrive"].as<bool>();
}

void BranchPhysicsParameters::OnInspect() {
    if (ImGui::TreeNodeEx("Physics")) {
        ImGui::DragFloat("Internode Density", &m_density, 0.1f, 0.01f, 1000.0f);
        ImGui::DragFloat2("RigidBody Damping", &m_linearDamping, 0.1f, 0.01f,
                          1000.0f);
        ImGui::DragFloat2("Drive Stiffness", &m_jointDriveStiffnessFactor, 0.1f,
                          0.01f, 1000000.0f);
        ImGui::DragFloat2("Drive Damping", &m_jointDriveDampingFactor, 0.1f, 0.01f,
                          1000000.0f);
        ImGui::Checkbox("Use acceleration", &m_enableAccelerationForDrive);

        int pi = m_positionSolverIteration;
        int vi = m_velocitySolverIteration;
        if (ImGui::DragInt("Velocity solver iteration", &vi, 1, 1, 100)) {
            m_velocitySolverIteration = vi;
        }
        if (ImGui::DragInt("Position solver iteration", &pi, 1, 1, 100)) {
            m_positionSolverIteration = pi;
        }
        ImGui::TreePop();
    }
}

void Branch::OnInspect() {
    m_branchPhysicsParameters.OnInspect();
}

void Branch::Serialize(YAML::Emitter &out) {
    m_currentRoot.Save("m_currentRoot", out);
    m_currentInternode.Save("m_currentInternode", out);
    out << YAML::Key << "m_branchPhysicsParameters" << YAML::Value << YAML::BeginMap;
    m_branchPhysicsParameters.Serialize(out);
    out << YAML::EndMap;
}

void Branch::Deserialize(const YAML::Node &in) {
    m_currentRoot.Load("m_currentRoot", in);
    m_currentInternode.Load("m_currentInternode", in);
    if (in["m_branchPhysicsParameters"]) m_branchPhysicsParameters.Deserialize(in["m_branchPhysicsParameters"]);
}

void Branch::Relink(const std::unordered_map<Handle, Handle> &map, const std::shared_ptr<Scene> &scene) {
    m_currentRoot.Relink(map);
    m_currentInternode.Relink(map);
}


