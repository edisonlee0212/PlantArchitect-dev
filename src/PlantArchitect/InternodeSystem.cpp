//
// Created by lllll on 8/27/2021.
//

#include "InternodeSystem.hpp"
#include "DefaultInternodeBehaviour.hpp"
#include "SpaceColonizationBehaviour.hpp"

using namespace PlantArchitect;

void InternodeSystem::Simulate(float deltaTime) {
    //0. Pre processing
    for (auto &i: m_internodeBehaviours) {
        auto behaviour = i.Get<IInternodeBehaviour>();
        if (behaviour) behaviour->PreProcess();
    }

    //1. Collect resource from environment
    EntityManager::ForEach<InternodeInfo>(JobManager::PrimaryWorkers(), m_internodesQuery,
                                          [=](int i, Entity entity, InternodeInfo &tag) {
                                              entity.GetOrSetPrivateComponent<Internode>().lock()->CollectResource(
                                                      deltaTime);
                                          }, true);
    //2. Upstream resource
    EntityManager::ForEach<InternodeInfo>(JobManager::PrimaryWorkers(), m_internodesQuery,
                                          [=](int i, Entity entity, InternodeInfo &tag) {
                                              entity.GetOrSetPrivateComponent<Internode>().lock()->UpStreamResource(
                                                      deltaTime);
                                          }, true);
    //3. Downstream resource
    EntityManager::ForEach<InternodeInfo>(JobManager::PrimaryWorkers(), m_internodesQuery,
                                          [=](int i, Entity entity, InternodeInfo &tag) {
                                              entity.GetOrSetPrivateComponent<Internode>().lock()->DownStreamResource(
                                                      deltaTime);
                                          }, true);

    //4. Growth
    for (auto &i: m_internodeBehaviours) {
        auto behaviour = i.Get<IInternodeBehaviour>();
        if (behaviour) behaviour->Grow();
    }

    //5. Post processing
    for (auto &i: m_internodeBehaviours) {
        auto behaviour = i.Get<IInternodeBehaviour>();
        if (behaviour) behaviour->PostProcess();
    }
}

#pragma region Methods

void InternodeSystem::OnInspect() {
    if (ImGui::Button("Test")) {
        Simulate(1.0f);
    }

    if (ImGui::TreeNodeEx("Internode Behaviours", ImGuiTreeNodeFlags_DefaultOpen)) {
        BehaviourSlotButton();
        int index = 0;
        bool skip = false;
        for (auto &i: m_internodeBehaviours) {
            if (EditorManager::DragAndDropButton<IAsset>(i, "Slot " + std::to_string(index++))) {
                skip = true;
                break;
            }
        }
        if (skip) {
            int index = 0;
            for (auto &i: m_internodeBehaviours) {
                if (!i.Get<IInternodeBehaviour>()) {
                    m_internodeBehaviours.erase(m_internodeBehaviours.begin() + index);
                    break;
                }
                index++;
            }
        }
        ImGui::TreePop();
    }
}

void InternodeSystem::BehaviourSlotButton() {
    ImGui::Text("Drop Behaviour");
    ImGui::SameLine();
    ImGui::Button("Here");
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DefaultInternodeBehaviour")) {
            IM_ASSERT(payload->DataSize == sizeof(std::shared_ptr<IAsset>));
            std::shared_ptr<DefaultInternodeBehaviour> payload_n =
                    std::dynamic_pointer_cast<DefaultInternodeBehaviour>(
                            *static_cast<std::shared_ptr<IAsset> *>(payload->Data));
            if (payload_n.get()) {
                bool search = false;
                for (auto &i: m_internodeBehaviours) {
                    if (i.Get<IInternodeBehaviour>()->GetTypeName() == "DefaultInternodeBehaviour") search = true;
                }
                if (!search) {
                    m_internodeBehaviours.emplace_back(payload_n);
                }
            }
        }
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SpaceColonizationBehaviour")) {
            IM_ASSERT(payload->DataSize == sizeof(std::shared_ptr<IAsset>));
            std::shared_ptr<SpaceColonizationBehaviour> payload_n =
                    std::dynamic_pointer_cast<SpaceColonizationBehaviour>(
                            *static_cast<std::shared_ptr<IAsset> *>(payload->Data));
            if (payload_n.get()) {
                bool search = false;
                for (auto &i: m_internodeBehaviours) {
                    if (i.Get<IInternodeBehaviour>()->GetTypeName() == "SpaceColonizationBehaviour") search = true;
                }
                if (!search) {
                    m_internodeBehaviours.emplace_back(payload_n);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void InternodeSystem::OnCreate() {
    m_internodesQuery = EntityManager::CreateEntityQuery();
    m_internodesQuery.SetAllFilters(InternodeInfo());

#pragma region Internode camera
    m_internodeDebuggingCamera =
            SerializationManager::ProduceSerializable<Camera>();
    m_internodeDebuggingCamera->m_useClearColor = true;
    m_internodeDebuggingCamera->m_clearColor = glm::vec3(0.1f);
    m_internodeDebuggingCamera->OnCreate();
#pragma endregion
    Enable();
}


void InternodeSystem::LateUpdate() {
    if (m_rightMouseButtonHold &&
        !InputManager::GetMouseInternal(GLFW_MOUSE_BUTTON_RIGHT,
                                        WindowManager::GetWindow())) {
        m_rightMouseButtonHold = false;
        m_startMouse = false;
    }
    m_internodeDebuggingCamera->ResizeResolution(
            m_internodeDebuggingCameraResolutionX,
            m_internodeDebuggingCameraResolutionY);
    m_internodeDebuggingCamera->Clear();

#pragma region Internode debug camera
    Camera::m_cameraInfoBlock.UpdateMatrices(
            EditorManager::GetInstance().m_sceneCamera,
            EditorManager::GetInstance().m_sceneCameraPosition,
            EditorManager::GetInstance().m_sceneCameraRotation);
    Camera::m_cameraInfoBlock.UploadMatrices(
            EditorManager::GetInstance().m_sceneCamera);
#pragma endregion

#pragma region Rendering
    if (m_drawBranches) {
        UpdateBranchColors();
        UpdateBranchCylinder(m_connectionWidth);
        if (m_internodeDebuggingCamera->IsEnabled())
            RenderBranchCylinders();
    }
    if (m_drawPointers) {
        UpdateBranchPointer(m_pointerLength, m_pointerWidth);
        if (m_internodeDebuggingCamera->IsEnabled())
            RenderBranchPointers();
    }
#pragma endregion

#pragma region Internode debugging camera
    ImVec2 viewPortSize;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Internodes");
    {
        if (ImGui::BeginChild("CameraRenderer", ImVec2(0, 0), false,
                              ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Settings")) {
#pragma region Menu
                    ImGui::Checkbox("Connections", &m_drawBranches);
                    if (m_drawBranches) {
                        if (ImGui::TreeNodeEx("Connection settings",
                                              ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::SliderFloat("Alpha", &m_transparency, 0, 1);
                            ImGui::DragFloat("Connection width", &m_connectionWidth, 0.01f, 0.01f, 1.0f);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::Checkbox("Pointers", &m_drawPointers);
                    if (m_drawPointers) {
                        if (ImGui::TreeNodeEx("Pointer settings",
                                              ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::ColorEdit4("Pointer color", &m_pointerColor.x);
                            ImGui::DragFloat("Pointer length", &m_pointerLength, 0.01f, 0.01f, 3.0f);
                            ImGui::DragFloat("Pointer width", &m_pointerWidth, 0.01f, 0.01f, 1.0f);
                            ImGui::TreePop();
                        }
                    }
#pragma endregion
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            viewPortSize = ImGui::GetWindowSize();
            viewPortSize.y -= 20;
            if (viewPortSize.y < 0)
                viewPortSize.y = 0;
            m_internodeDebuggingCameraResolutionX = viewPortSize.x;
            m_internodeDebuggingCameraResolutionY = viewPortSize.y;
            ImGui::Image(
                    reinterpret_cast<ImTextureID>(
                            m_internodeDebuggingCamera->GetTexture()->Texture()->Id()),
                    viewPortSize, ImVec2(0, 1), ImVec2(1, 0));
            glm::vec2 mousePosition = glm::vec2(FLT_MAX, FLT_MIN);
            if (ImGui::IsWindowFocused()) {
                bool valid = true;
                mousePosition = InputManager::GetMouseAbsolutePositionInternal(
                        WindowManager::GetWindow());
                float xOffset = 0;
                float yOffset = 0;
                if (valid) {
                    if (!m_startMouse) {
                        m_lastX = mousePosition.x;
                        m_lastY = mousePosition.y;
                        m_startMouse = true;
                    }
                    xOffset = mousePosition.x - m_lastX;
                    yOffset = -mousePosition.y + m_lastY;
                    m_lastX = mousePosition.x;
                    m_lastY = mousePosition.y;
#pragma region Scene Camera Controller
                    if (!m_rightMouseButtonHold &&
                        InputManager::GetMouseInternal(GLFW_MOUSE_BUTTON_RIGHT,
                                                       WindowManager::GetWindow())) {
                        m_rightMouseButtonHold = true;
                    }
                    if (m_rightMouseButtonHold &&
                        !EditorManager::GetInstance().m_lockCamera) {
                        glm::vec3 front =
                                EditorManager::GetInstance().m_sceneCameraRotation *
                                glm::vec3(0, 0, -1);
                        glm::vec3 right =
                                EditorManager::GetInstance().m_sceneCameraRotation *
                                glm::vec3(1, 0, 0);
                        if (InputManager::GetKeyInternal(GLFW_KEY_W,
                                                         WindowManager::GetWindow())) {
                            EditorManager::GetInstance().m_sceneCameraPosition +=
                                    front * static_cast<float>(Application::Time().DeltaTime()) *
                                    EditorManager::GetInstance().m_velocity;
                        }
                        if (InputManager::GetKeyInternal(GLFW_KEY_S,
                                                         WindowManager::GetWindow())) {
                            EditorManager::GetInstance().m_sceneCameraPosition -=
                                    front * static_cast<float>(Application::Time().DeltaTime()) *
                                    EditorManager::GetInstance().m_velocity;
                        }
                        if (InputManager::GetKeyInternal(GLFW_KEY_A,
                                                         WindowManager::GetWindow())) {
                            EditorManager::GetInstance().m_sceneCameraPosition -=
                                    right * static_cast<float>(Application::Time().DeltaTime()) *
                                    EditorManager::GetInstance().m_velocity;
                        }
                        if (InputManager::GetKeyInternal(GLFW_KEY_D,
                                                         WindowManager::GetWindow())) {
                            EditorManager::GetInstance().m_sceneCameraPosition +=
                                    right * static_cast<float>(Application::Time().DeltaTime()) *
                                    EditorManager::GetInstance().m_velocity;
                        }
                        if (InputManager::GetKeyInternal(GLFW_KEY_LEFT_SHIFT,
                                                         WindowManager::GetWindow())) {
                            EditorManager::GetInstance().m_sceneCameraPosition.y +=
                                    EditorManager::GetInstance().m_velocity *
                                    static_cast<float>(Application::Time().DeltaTime());
                        }
                        if (InputManager::GetKeyInternal(GLFW_KEY_LEFT_CONTROL,
                                                         WindowManager::GetWindow())) {
                            EditorManager::GetInstance().m_sceneCameraPosition.y -=
                                    EditorManager::GetInstance().m_velocity *
                                    static_cast<float>(Application::Time().DeltaTime());
                        }
                        if (xOffset != 0.0f || yOffset != 0.0f) {
                            EditorManager::GetInstance().m_sceneCameraYawAngle +=
                                    xOffset * EditorManager::GetInstance().m_sensitivity;
                            EditorManager::GetInstance().m_sceneCameraPitchAngle +=
                                    yOffset * EditorManager::GetInstance().m_sensitivity;
                            if (EditorManager::GetInstance().m_sceneCameraPitchAngle > 89.0f)
                                EditorManager::GetInstance().m_sceneCameraPitchAngle = 89.0f;
                            if (EditorManager::GetInstance().m_sceneCameraPitchAngle < -89.0f)
                                EditorManager::GetInstance().m_sceneCameraPitchAngle = -89.0f;

                            EditorManager::GetInstance().m_sceneCameraRotation =
                                    Camera::ProcessMouseMovement(
                                            EditorManager::GetInstance().m_sceneCameraYawAngle,
                                            EditorManager::GetInstance().m_sceneCameraPitchAngle,
                                            false);
                        }
                    }
#pragma endregion
                    if (m_drawBranches) {
#pragma region Ray selection
                        m_currentFocusingInternode = Entity();
                        std::mutex writeMutex;
                        auto windowPos = ImGui::GetWindowPos();
                        auto windowSize = ImGui::GetWindowSize();
                        mousePosition.x -= windowPos.x;
                        mousePosition.x -= windowSize.x;
                        mousePosition.y -= windowPos.y + 20;
                        float minDistance = FLT_MAX;
                        GlobalTransform cameraLtw;
                        cameraLtw.m_value =
                                glm::translate(
                                        EditorManager::GetInstance().m_sceneCameraPosition) *
                                glm::mat4_cast(
                                        EditorManager::GetInstance().m_sceneCameraRotation);
                        const Ray cameraRay = m_internodeDebuggingCamera->ScreenPointToRay(
                                cameraLtw, mousePosition);
                        EntityManager::ForEach<GlobalTransform, InternodeInfo>(
                                JobManager::PrimaryWorkers(),
                                m_internodesQuery,
                                [&, cameraLtw, cameraRay](int i, Entity entity,
                                                          GlobalTransform &ltw,
                                                          InternodeInfo &internodeInfo) {
                                    const glm::vec3 position = ltw.m_value[3];
                                    const glm::vec3 position2 = position + internodeInfo.m_length * glm::normalize(
                                            ltw.GetRotation() * glm::vec3(0, 0, -1));
                                    const auto center = (position + position2) / 2.0f;
                                    auto dir = cameraRay.m_direction;
                                    auto pos = cameraRay.m_start;
                                    const auto radius = internodeInfo.m_thickness;
                                    const auto height = glm::distance(position2, position);
                                    if (!cameraRay.Intersect(center, height / 2.0f))
                                        return;

#pragma region Line Line intersection
                                    /*
                 * http://geomalgorithms.com/a07-_distance.html
                 */
                                    glm::vec3 u = pos - (pos + dir);
                                    glm::vec3 v = position - position2;
                                    glm::vec3 w = (pos + dir) - position2;
                                    const auto a = dot(u,
                                                       u); // always >= 0
                                    const auto b = dot(u, v);
                                    const auto c = dot(v,
                                                       v); // always >= 0
                                    const auto d = dot(u, w);
                                    const auto e = dot(v, w);
                                    const auto dotP = a * c - b * b; // always >= 0
                                    float sc, tc;
                                    // compute the line parameters of the two closest points
                                    if (dotP < 0.001f) { // the lines are almost parallel
                                        sc = 0.0f;
                                        tc = (b > c ? d / b
                                                    : e / c); // use the largest denominator
                                    } else {
                                        sc = (b * e - c * d) / dotP;
                                        tc = (a * e - b * d) / dotP;
                                    }
                                    // get the difference of the two closest points
                                    glm::vec3 dP = w + sc * u - tc * v; // =  L1(sc) - L2(tc)
                                    if (glm::length(dP) > radius)
                                        return;
#pragma endregion

                                    const auto distance = glm::distance(
                                            glm::vec3(cameraLtw.m_value[3]), glm::vec3(center));
                                    std::lock_guard<std::mutex> lock(writeMutex);
                                    if (distance < minDistance) {
                                        minDistance = distance;
                                        m_currentFocusingInternode = entity;
                                    }
                                });
                        if (InputManager::GetMouseInternal(GLFW_MOUSE_BUTTON_LEFT,
                                                           WindowManager::GetWindow())) {
                            if (!m_currentFocusingInternode.Get().IsNull()) {
                                EditorManager::SetSelectedEntity(
                                        m_currentFocusingInternode.Get());
                            }
                        }
#pragma endregion
                    }
                }
            }
        }
        ImGui::EndChild();
        auto *window = ImGui::FindWindowByName("Internodes");
        m_internodeDebuggingCamera->SetEnabled(
                !(window->Hidden && !window->Collapsed));
    }
    ImGui::End();
    ImGui::PopStyleVar();

#pragma endregion
}

void InternodeSystem::UpdateBranchColors() {
    auto focusingInternode = Entity();
    auto selectedEntity = Entity();
    if (m_currentFocusingInternode.Get().IsValid()) {
        focusingInternode = m_currentFocusingInternode.Get();
    }
    if (EditorManager::GetSelectedEntity().IsValid()) {
        selectedEntity = EditorManager::GetSelectedEntity();
    }
    EntityManager::ForEach<BranchColor, InternodeInfo>(
            JobManager::PrimaryWorkers(),
            m_internodesQuery,
            [=](int i, Entity entity, BranchColor &internodeRenderColor,
                InternodeInfo &internodeInfo) {
                internodeRenderColor.m_value = glm::vec4(m_branchColor, m_transparency);
            },
            false);
    BranchColor color;
    color.m_value = glm::vec4(1, 1, 1, 1);
    if (focusingInternode.IsValid() && focusingInternode.HasDataComponent<BranchColor>()) focusingInternode.SetDataComponent(color);
    color.m_value = glm::vec4(1, 0, 0, 1);
    if (selectedEntity.IsValid() && selectedEntity.HasDataComponent<BranchColor>()) selectedEntity.SetDataComponent(color);
}

void InternodeSystem::UpdateBranchCylinder(const float &width) {
    EntityManager::ForEach<GlobalTransform, BranchCylinder, BranchCylinderWidth, InternodeInfo>(
            JobManager::PrimaryWorkers(),
            m_internodesQuery,
            [width](int i, Entity entity, GlobalTransform &ltw, BranchCylinder &c,
                    BranchCylinderWidth &branchCylinderWidth, InternodeInfo &internodeInfo) {
                glm::vec3 scale;
                glm::quat rotation;
                glm::vec3 translation;
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(ltw.m_value, scale, rotation, translation, skew,
                               perspective);
                const auto direction = glm::normalize(rotation * glm::vec3(0, 0, -1));
                const glm::vec3 position2 =
                        translation + internodeInfo.m_length * direction;
                rotation = glm::quatLookAt(
                        direction, glm::vec3(direction.y, direction.z, direction.x));
                rotation *= glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
                const glm::mat4 rotationTransform = glm::mat4_cast(rotation);
                branchCylinderWidth.m_value = internodeInfo.m_thickness;
                c.m_value =
                        glm::translate((translation + position2) / 2.0f) *
                        rotationTransform *
                        glm::scale(glm::vec3(
                                branchCylinderWidth.m_value,
                                glm::distance(translation, position2) / 2.0f,
                                internodeInfo.m_thickness));

            },
            false);
}

void InternodeSystem::UpdateBranchPointer(const float &length, const float &width) {


}

void InternodeSystem::RenderBranchCylinders() {
    std::vector<BranchCylinder> branchCylinders;
    m_internodesQuery.ToComponentDataArray<BranchCylinder>(
            branchCylinders);
    std::vector<BranchColor> branchColors;
    m_internodesQuery.ToComponentDataArray<BranchColor>(
            branchColors);
    if (!branchCylinders.empty())
        RenderManager::DrawGizmoMeshInstancedColored(
                DefaultResources::Primitives::Cylinder, m_internodeDebuggingCamera,
                EditorManager::GetInstance().m_sceneCameraPosition,
                EditorManager::GetInstance().m_sceneCameraRotation,
                *reinterpret_cast<std::vector<glm::vec4> *>(&branchColors),
                *reinterpret_cast<std::vector<glm::mat4> *>(&branchCylinders),
                glm::mat4(1.0f), 1.0f);
}

void InternodeSystem::RenderBranchPointers() {
    std::vector<BranchPointer> branchPointers;
    m_internodesQuery.ToComponentDataArray<BranchPointer>(
            branchPointers);
    if (!branchPointers.empty())
        RenderManager::DrawGizmoMeshInstanced(
                DefaultResources::Primitives::Cylinder, m_internodeDebuggingCamera,
                EditorManager::GetInstance().m_sceneCameraPosition,
                EditorManager::GetInstance().m_sceneCameraRotation, m_pointerColor,
                *reinterpret_cast<std::vector<glm::mat4> *>(&branchPointers),
                glm::mat4(1.0f), 1.0f);
}

#pragma endregion
