import {Chip} from "@nextui-org/react";
import {useEffect} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../../ros/topics/rosTopic.ts";

interface AutoStatusProps {
}

const variants: [string, string, string][] = [
  ["Idle", "border-[#3eb1cf]", "bg-[#3eb1cf]"],
  ["Traversing", "border-primary", "bg-primary"],
  ["Searching", "border-warning", "bg-warning"],
  ["Arrived Successfully", "border-success", "bg-success"],
  ["Arrived Unsuccessfully", "border-danger", "bg-danger"],
];

export const AutoStatus : React.FC<AutoStatusProps> = () => {
  const bifrost = useBifrost({topic: RosTopic.AUTO_STATUS})
  const autoStatus = useSelector((state: RootState) => state.autoStatus.status)

  useEffect(() => {
    bifrost.syncWithTopic()
  }, [bifrost]);

  return (
    <Chip
      key={autoStatus}
      radius='md'
      size="lg"
      variant="dot"
      classNames={{
        base: `h-10 border-3 ${variants[autoStatus][1]}`,
        dot: variants[autoStatus][2]
      }}
    >
      {variants[autoStatus][0]}
    </Chip>
  )
}

export default AutoStatus;
