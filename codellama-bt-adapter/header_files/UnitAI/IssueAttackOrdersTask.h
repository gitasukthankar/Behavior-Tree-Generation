//--|====================================================================|--
//--| CLASSIFICATION: UNCLASSIFIED
//--|====================================================================|--
//--| This program is the sole property of the Lockheed Martin Corporation
//--| and contains proprietary and confidential information. Use or
//--| disclosure of this program is subject to the terms and conditions of
//--| a license agreement with the Lockheed Martin Corporation. Unauthorized
//--| use or distribution will be subject to action as prescribed by the
//--| license agreement.
//--|
//--| Copyright 2023 by Lockheed Martin Corporation
//--|====================================================================|--
//--|
//--| Description:
//--| Defines the FIssueAttackOrdersTask struct and supporting types.
//--|
//--|====================================================================|--
#pragma once

// MilVerse
#include "AI/MilVerseStateTreeTask.h"
#include "Components/Orders/AttackSuborderComponent.h"
#include "Components/UnitControllerComponent.h"
#include "Components/Units/SegmentedRouteComponent.h"
#include "Components/Units/UnitFormationComponent.h"
#include "Routes/RoutePoint.h"
// Unreal Engine
#include "CoreMinimal.h"
#include "IssueAttackOrdersTask.generated.h"

//--------------------------------------------------------------------------------------------------

class UAttackOrder;
class UOrder;
class UOrderSubsystem;

//--------------------------------------------------------------------------------------------------

/// @brief Specifies the stages of issuing attack orders to lower echelons.
///
/// @ingroup SimulationBehaviors-Module
UENUM()
enum class EIssueAttackOrdersStage : uint8
{
    CREATE_LEADER_ORDER,         ///< @brief Order being created for the leader.
    VALIDATING_LEADER_ORDER,     ///< @brief Waiting for leader order validation to complete.
    EXECUTING_LEADER_ORDER,      ///< @brief Sending order to leader.
    CREATE_FOLLOWER_ORDERS,      ///< @brief Order being created for the followers.
    VALIDATING_FOLLOWER_ORDERS,  ///< @brief Waiting for all follower orders to be validated.
    EXECUTING_FOLLOWER_ORDERS,   ///< @brief Sending orders to followers.
    COMPLETE,                    ///< @brief Task completed successfully.
    FAILED                       ///< @brief Task failed.
};

//--------------------------------------------------------------------------------------------------

/// @brief Instance data for @ref FIssueAttackOrdersTask
///
/// @ingroup SimulationBehaviors-Module
USTRUCT()
struct SIMULATIONBEHAVIORS_API FIssueAttackOrdersTaskInstanceData
{
    GENERATED_BODY()

    /// @brief Parent attack order.
    UPROPERTY(VisibleAnywhere, Category = Input)
    TObjectPtr<UOrder> ParentOrder = 0;

    /// @brief TSet of healthy entities (health > 0)
    UPROPERTY(VisibleAnywhere, Category = Input)
    TSet<FGuid> HealthyEntities;

    /// @brief Flag to skip "Formup" behavior
    UPROPERTY(VisibleAnywhere, Category = Input)
    bool bShouldSkipFormup = true;

    /// @brief The current stage of this task.
    UPROPERTY()
    EIssueAttackOrdersStage Stage = EIssueAttackOrdersStage::CREATE_LEADER_ORDER;
};

//--------------------------------------------------------------------------------------------------

/// @brief Task for issuing attack orders to lower echelon units/individuals.
///
/// This task uses the following ECS components. The task will not fail if they are missing, but
/// will not perform any actions until all are added.
///
/// @ref FSegmentedRouteComponent
/// @ref FUnitFormationComponent
/// @ref FAttackSuborderComponent
///
/// @ingroup SimulationBehaviors-Module
USTRUCT(meta = (MilVerseUnitLevel))
struct SIMULATIONBEHAVIORS_API FIssueAttackOrdersTask : public FMilVerseStateTreeTask
{
    GENERATED_BODY()

    /// @brief Alias for this node's instance data type.
    using FInstanceDataType = FIssueAttackOrdersTaskInstanceData;

protected:
    /// @brief Called when the state tree asset is linked with data to allow the task to resolve
    /// references to other state tree data.
    ///
    /// @param Linker
    ///     Reference to the state tree's linker.
    /// @returns
    ///     Returns `true` if linking is successful; Otherwise returns `false`.
    bool Link(FStateTreeLinker& Linker) override;

    /// @brief Returns the struct definition for this task's instance data.
    ///
    /// @sa FIssueAttackOrdersTaskInstanceData
    const UStruct* GetInstanceDataType() const override
    {
        return FInstanceDataType::StaticStruct();
    }

    /// @brief Called when the state is entered that this task is part of.
    ///
    /// @param Context
    ///     The state tree context.
    /// @param Transition
    ///     Information about the transition that caused the state to be selected. Not used for this
    ///     task.
    /// @returns
    ///     The running status of the task.
    EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) const override;

    /// @brief Called when a current state is exited and task is part of active states.
    ///
    /// @param Context
    ///     The state tree context.
    /// @param Transition Describes the states involved in the transition
    ///     Information about the transition that caused the state to be exited. Not used for this
    ///     task.
    void ExitState(FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) const override;

    /// @brief Called during the state tree tick when the task is part of an active state.
    ///
    /// @param Context
    ///     The state tree context.
    /// @param DeltaTime
    ///     The time in seconds since `Tick` was last called.
    /// @returns
    ///     The running status of the task.
    EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
        const float DeltaTime) const override;

protected:
    /// @brief Handle for the @ref FSegmentedRouteComponent ECS component.
    TOptionalStateTreeExternalDataHandle<FSegmentedRouteComponent> SegmentedRouteHandle;

    /// @brief Data handle for the @ref FUnitFormationComponent ECS component.
    TOptionalStateTreeExternalDataHandle<FUnitFormationComponent> UnitFormationHandle;

    /// @brief Data handle for the @ref FAttackSuborderComponent ECS component.
    TOptionalStateTreeExternalDataHandle<FAttackSuborderComponent> AttackSuborderHandle;

    /// @brief Data handle for the @ref FUnitControllerComponent ECS component.
    TOptionalStateTreeExternalDataHandle<FUnitControllerComponent> UnitControllerHandle;

private:
    /// @brief Creates the leader's attack order and requests its validation.
    ///
    /// @param Context
    ///     The state tree context.
    /// @param OrderSubsystem
    ///     The order subsystem which is used to created, validate, and execute orders.
    /// @param InstanceData
    ///     Instance data for the state tree instance being processed.
    /// @param SegmentedRoute
    ///     Component containing segmented route information.
    /// @param UnitFormation
    ///     Component containing data about the current formation.
    /// @param AttackSuborder
    ///     Component containing information about suborders generated as part of executing the
    ///     move tactically order. This component will be populated with the leader order that is
    ///     created.
    void CreateLeaderOrder(FStateTreeExecutionContext& Context,
        UOrderSubsystem* OrderSubsystem,
        FInstanceDataType& InstanceData,
        const FSegmentedRouteComponent& SegmentedRoute,
        const FUnitFormationComponent& UnitFormation,
        FAttackSuborderComponent& AttackSuborder) const;

    /// @brief Waits for the leader's attack order to complete validation.
    ///
    /// @param InstanceData
    ///     Instance data for the state tree instance being processed.
    /// @param AttackSuborder
    ///     Component containing information about suborders generated as part of executing the
    ///     move tactically order.
    void WaitForLeaderOrderValidation(FInstanceDataType& InstanceData,
        const FAttackSuborderComponent& AttackSuborder) const;

    /// @brief Sends the leader's attack order.
    ///
    /// @param OrderSubsystem
    ///     The order subsystem which is used to created, validate, and execute orders.
    /// @param InstanceData
    ///     Instance data for the state tree instance being processed.
    /// @param AttackSuborder
    ///     Component containing information about suborders generated as part of executing the
    ///     move tactically order.
    void ExecuteLeaderOrder(UOrderSubsystem* OrderSubsystem,
        FInstanceDataType& InstanceData,
        const FAttackSuborderComponent& AttackSuborder) const;

    /// @brief Creates the follower orders for the non-leaders and requests their validation.
    ///
    /// @param Context
    ///     The state tree context.
    /// @param OrderSubsystem
    ///     The order subsystem which is used to created, validate, and execute orders.
    /// @param InstanceData
    ///     Instance data for the state tree instance being processed.
    /// @param SegmentedRoute
    ///    Route to be followed by the orders.
    /// @param UnitFormation
    ///     Component containing data about the current formation.
    /// @param AttackSuborder
    ///     Component containing information about suborders generated as part of executing the
    ///     move tactically order.
    void CreateFollowerOrders(FStateTreeExecutionContext& Context,
        UOrderSubsystem* OrderSubsystem,
        FInstanceDataType& InstanceData,
        const FSegmentedRouteComponent& SegmentedRoute,
        const FUnitFormationComponent& UnitFormation,
        FAttackSuborderComponent& AttackSuborder) const;

    /// @brief Waits for the follower's attack order to complete validation.
    ///
    /// @param InstanceData
    ///     Instance data for the state tree instance being processed.
    /// @param AttackSuborder
    ///     Component containing information about suborders generated as part of executing the
    ///     move tactically order.
    void WaitForFollowerOrderValidations(FInstanceDataType& InstanceData,
        FAttackSuborderComponent& AttackSuborder) const;

    /// @brief Sends the follower orders.
    ///
    /// @param OrderSubsystem
    ///     The order subsystem which is used to created, validate, and execute orders.
    /// @param InstanceData
    ///     Instance data for the state tree instance being processed.
    /// @param AttackSuborder
    ///     Component containing information about suborders generated as part of executing the
    ///     move tactically order.
    void ExecuteFollowerOrders(UOrderSubsystem* OrderSubsystem,
        FInstanceDataType& InstanceData,
        FAttackSuborderComponent& AttackSuborder) const;

private:
    /// @brief If paranet order is not null and is currently executing, fail the order.
    ///
    /// @param InstanceData
    ///     Instance data for the current entity being processed by the state tree.
    /// @param Reason
    ///     Reason for the failure. Should be human readable.
    void FailOrder(const FInstanceDataType& InstanceData, const FText& Reason) const;
};
