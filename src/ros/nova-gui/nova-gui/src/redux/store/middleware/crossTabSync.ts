import {Action, ListenerEffectAPI, UnknownAction} from '@reduxjs/toolkit'
import {v4 as uuidv4} from "uuid";
import {BroadcastChannel} from "broadcast-channel";
import {Dispatch} from "redux";
import {BifrostActionTypes} from "../../actions/bifrost/createBifrostAction.ts";

const SYNC_CHANNEL_NAME = "tab-sync-channel";

// Message that is broadcast when a sync occurs
export interface Message {
  senderId: string,
  action: Action,
}

export interface TabSyncBlacklist {
  stores: string[],
  actions: string[],
}

const BIFROST_COMPONENTS = [
  BifrostActionTypes.UPDATE_TOPIC_STATE.toString(),
  BifrostActionTypes.UPDATE_SERVICE_STATE.toString(),
  BifrostActionTypes.INITIATE_CONTACT.toString(),
  BifrostActionTypes.CONNECTION_UPDATE.toString(),
  BifrostActionTypes.SUBSCRIBE_TOPIC.toString(),
]

/**
 * Handler for broadcast messages, when a message is received it will dispatch the associated action.
 * @param myID ID of this tab, to ensure the tab does not dispatch the sync messages it sends.
 * @param dispatch function to dispatch to the redux store
 */
const syncMessageListener = <D extends Dispatch>(myID: string, dispatch: D) => {
  return (msg: Message) => {
    if (msg.senderId != myID) {
      // adds $isSync field to signal to middleware that this action should not be broadcast.
      dispatch({...msg.action, $isSync: true} as UnknownAction);
    }
  }
}

/**
 * Initialises the tab's ID and the broadcast channel
 * For every whitelisted action that is received, if it is not a sync action, then it
 * will broadcast the message to the sync channel.
 * @return the effect of the middleware, a function that is called for every whitelisted action.
 */
export const tabSyncMiddleware = <S, D extends Dispatch>() => {
  const myID = uuidv4();
  const channel = new BroadcastChannel(SYNC_CHANNEL_NAME);
  let addedEventListener = false;

  return (action: Action, listenerAPI: ListenerEffectAPI<S, D>) => {

    // add the event listener only once when there is access to dispatch
    if (!addedEventListener)
      channel.addEventListener("message", syncMessageListener(myID, listenerAPI.dispatch));
      addedEventListener = true

    // only post the message when it is not a sync action
    if (!("$isSync" in action)) {
      channel.postMessage({
        senderID: myID,
        action: action,
      });
    }
  }
}

/**
 * skips any blacklisted stores and actions, and bifrost actions
 * @param blacklist blacklist of stores and actions
 */
export const tabSyncPredicate = (blacklist: TabSyncBlacklist) => (action: Action) => {
  // skip any blacklisted actions
  if (blacklist.actions.includes(action.type))
    return false

  const actionParts = action.type.split('/');

  if (actionParts.length < 2)
    return false

  // skip any blacklisted stores and bifrost stores
  return !((blacklist.stores.includes(actionParts[0]) || (BIFROST_COMPONENTS.includes(actionParts[1]))))
}
