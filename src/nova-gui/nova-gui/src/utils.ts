import {IRosSensorMsgsJointState} from "./ros/rosTypes.ts";

const getJointState = (name: string, names: string[], states: number[]) : number => {
  const index = names.indexOf(name);
  if (index >= 0) {
    return states[index];
  }
  return 0.0;
}

export function getJointEffort(joint: string, jointState: IRosSensorMsgsJointState)  {
  return getJointState(joint, jointState.name, jointState.effort);
}
export function getJointVelocity(joint: string, jointState: IRosSensorMsgsJointState)  {
  return getJointState(joint, jointState.name, jointState.velocity);
}
export function getJointPosition(joint: string, jointState: IRosSensorMsgsJointState)  {
  return getJointState(joint, jointState.name, jointState.position);
}