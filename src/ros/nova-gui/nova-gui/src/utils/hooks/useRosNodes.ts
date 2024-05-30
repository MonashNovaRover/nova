import { useContext, useEffect, useState } from "react";
import { RosContext } from "../../redux/context/ros/RosContext";

export const useRosNodes = () => {
  const [nodes, setNodes] = useState<string[]>([]);

  const ros = useContext(RosContext);

  useEffect(() => {
    if (ros) {
      ros.getNodes((nodeList) => setNodes(nodeList));
    }
  }, [ros]);

  return nodes;
};
