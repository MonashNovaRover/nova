import { Chip } from "@nextui-org/react";
import { IRosBlcmdInterfacesBlcmdStatus } from "../../../ros/rosTypes.ts";

export const ComplainingChips = (props: IRosBlcmdInterfacesBlcmdStatus) => {
   
  const { id, ...allComplaints } = props; // I'm just extracting everything except id

  const noComplaints = Object.values(allComplaints).every(
    (complaint: boolean) => !complaint
  );

  if (noComplaints) {
    return (
      <Chip variant="flat" size="sm" className="rounded-md" color="success">
        No Issues
      </Chip>
    );
  }

  return (
    <div className="flex flex-row gap-1">
      {/* I know code below is garbage, but it's 1 AM */}
      {allComplaints.gate_fault && (
        <Chip variant="flat" size="sm" className="rounded-md" color={"danger"}>
          {"Gate Fault"}
        </Chip>
      )}
      {allComplaints.overspeed_fault && (
        <Chip variant="flat" size="sm" className="rounded-md" color={"danger"}>
          {"Overspeed Fault"}
        </Chip>
      )}
      {allComplaints.resolver_fault && (
        <Chip variant="flat" size="sm" className="rounded-md" color={"danger"}>
          {"Resolver Fault"}
        </Chip>
      )}
      {allComplaints.stall_fault && (
        <Chip variant="flat" size="sm" className="rounded-md" color={"danger"}>
          {"Stall Fault"}
        </Chip>
      )}
    </div>
  );
};
