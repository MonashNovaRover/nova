import React, { useState, useMemo } from "react";
import { DndContext, closestCenter, PointerSensor, useSensor, useSensors, DragEndEvent } from "@dnd-kit/core";
import { arrayMove, SortableContext, verticalListSortingStrategy } from "@dnd-kit/sortable";
import { restrictToVerticalAxis, restrictToWindowEdges, restrictToFirstScrollableAncestor } from "@dnd-kit/modifiers";
import { Modal, ModalHeader, ModalFooter, ModalBody, ModalContent, Card, Divider } from "@nextui-org/react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../../ros/services/rosService.ts";
import { GoalType, MapPoint } from "../../../../redux/models/CartographerState.ts";
import { IRosNovaInterfacesCartographerCommandRequest } from "../../../../ros/rosTypes.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { SortablePoints, GoalPointCard } from "./GoalPointCards.tsx";
import { GoalPublishingFooter } from "./GoalPublishingFooter.tsx";

export const CartographerGoalModal: React.FC<{
  isOpen: boolean;
  onClose: () => void;
}> = ({ isOpen, onClose }) => {
  const sensors = useSensors(useSensor(PointerSensor));
  
  const serviceBifrost = useBifrost({
    service: RosService.CARTOGRAPHER_COMMAND,
  });

  const { points } = useSelector(
    (state: RootState) => state.cartographerState
  );
  
  const [selection, setSelection] = useState<number[]>([]);
  const [overrideGoalType, setOverrideGoalType] = useState<GoalType | null>(null);
  const [overrideSearchRadius, setOverrideSearchRadius] = useState<number | null>(null);
  const [isEditingFooter, setIsEditingFooter] = useState<boolean>(false);

  // Derive selected items
  const selectedItems = useMemo(() => selection.map((v) => points[v]), [selection, points]);

  // Derive default values from last selected point
  const defaultGoalType = useMemo(() => {
    if (selectedItems.length > 0) {
      const lastPoint = selectedItems[selectedItems.length - 1];
      return lastPoint?.labelNumber ?? GoalType.GNSS;
    }
    return GoalType.GNSS;
  }, [selectedItems]);

  const defaultSearchRadius = useMemo(() => {
    if (selectedItems.length > 0) {
      const lastPoint = selectedItems[selectedItems.length - 1];
      return lastPoint?.searchRadius ?? 0;
    }
    return 0;
  }, [selectedItems]);

  // Compute effective values (override or default)
  const effectiveGoalType = overrideGoalType ?? defaultGoalType;
  const effectiveSearchRadius = overrideSearchRadius ?? defaultSearchRadius;

  const resetOverrides = () => {
    setOverrideGoalType(null);
    setOverrideSearchRadius(null);
    setIsEditingFooter(false);
  };

  const handleDragEnd = (event: DragEndEvent) => {
    const { active, over } = event;
    if (active.id !== over!.id) {
      const oldIndex = selection.findIndex((p) => points[p].name === active.id);
      const newIndex = selection.findIndex((p) => points[p].name === over!.id);
      setSelection(arrayMove(selection, oldIndex, newIndex));
      resetOverrides();
    }
  };

  const toggleSelection = (name: string) => {
    const i = points.findIndex((v)=>v.name === name);
    if (selection.includes(i)) {
      setSelection((prev)=>prev.filter((v)=>v!==i))
    } else {
      setSelection((prev)=>[...prev, points.findIndex((v)=>v.name === name)])
    }
    resetOverrides();
  };

  if (!isOpen && selection.length > 0) setSelection([]);


  const sendCartographerPoints = () => {
    const selected = selection.map((v)=>points[v]);
    const goals = selected.map((item) => ({
      latitude: item.lat,
      longitude: item.long,
    }));

    // Use effective values (override or default)
    console.log("calling service with:", {
      goals: goals,
      goal_type: effectiveGoalType,
      search_radius: effectiveSearchRadius
    });
    serviceBifrost.callService({
      goals: goals,
      goal_type: effectiveGoalType,
      search_radius: effectiveSearchRadius
    } as IRosNovaInterfacesCartographerCommandRequest);
  };

  const renderPoints = (pointsList: MapPoint[], isSortable: boolean) => {
    return pointsList.map((point, index) => {
      const isLastSelected = isSortable && index === pointsList.length - 1;
      const isSelected = selection.includes(points.indexOf(point));

      return isSortable ? (
        <SortablePoints key={point.name} id={point.name} isSpecial={isLastSelected}>
          <GoalPointCard
            point={point}
            isSelected={isSelected}
            onToggleSelection={toggleSelection}
          />
        </SortablePoints>
      ) : (
        <Card
          key={point.name}
          className="p-4 mb-4 rounded-lg bg-gray-100 bg-neutral-800"
        >
          <GoalPointCard
            point={point}
            isSelected={isSelected}
            onToggleSelection={toggleSelection}
          />
        </Card>
      );
    });
  };

  const unselectedItems = points.filter((_, i) => !selection.includes(i));

  return (
    <Modal
      isOpen={isOpen}
      className="dark text-foreground"
      size="5xl"
      scrollBehavior="inside"
      onClose={onClose}
    >
      <ModalContent>
        <ModalHeader>Publish Goals</ModalHeader>
        <ModalBody>
          <div>
            <div>
              <DndContext
                sensors={sensors}
                collisionDetection={closestCenter}
                modifiers={[
                  restrictToVerticalAxis,
                  restrictToWindowEdges,
                  restrictToFirstScrollableAncestor,
                ]}
                onDragEnd={handleDragEnd}
              >
                <SortableContext
                  items={selectedItems.map((item) => item.name)}
                  strategy={verticalListSortingStrategy}
                >
                  <h3 className="my-4 font-bold">Selected Points</h3>
                  {renderPoints(selectedItems, true)}
                </SortableContext>
              </DndContext>
            </div>
            <Divider className="my-4" />
            <h3 className="font-bold my-4">Unselected Points</h3>
            {renderPoints(unselectedItems, false)}
          </div>
        </ModalBody>
        <ModalFooter>
          <GoalPublishingFooter
            goalType={effectiveGoalType}
            searchRadius={effectiveSearchRadius}
            hasSelection={selection.length > 0}
            isEditing={isEditingFooter}
            onGoalTypeChange={setOverrideGoalType}
            onSearchRadiusChange={setOverrideSearchRadius}
            onToggleEditing={() => setIsEditingFooter(!isEditingFooter)}
            onPublish={sendCartographerPoints}
          />
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
};