import { RootState } from "../models/RootState";

export enum BifrostActionTypes {
  UPDATE_DATA,
}

export interface BifrostActionType<P> {
  type: string;
  payload: P;
}

export function createBifrostAction<T>() {
  return {
    update(object: T): () => BifrostActionType<T> {
      return () => ({
        type: BifrostActionTypes.UPDATE_DATA.toString(),
        payload: object,
      });
    },
    updateStuff(object: T) {
      return async (dispatch: Function, getState: () => RootState) => {
        dispatch(this.update(object));
      };
    },
    // /**
    //  * Populates store & establishes WS for live-store updates
    //  * @param filters
    //  * @param wsFilter
    //  */
    // ensureConsistent(
    //   filters: QueryFilter<T>,
    //   wsFilter?: WSQueryFilter<T>,
    //   ignorePrevious: boolean = false
    // ) {
    //   return async (dispatch: Function, getState: () => RootState) => {
    //     const state = getState();
    //     const store: Y = selectFn(state);
    //     const filter = pickBy(filters, identity) as QueryFilter<T>;

    //     const response = await fetchData(
    //       filter,
    //       store.fetchHistory,
    //       dispatch,
    //       fetchFn,
    //       this._startFetch,
    //       this._errorFetch,
    //       this._finishFetch,
    //       ignorePrevious
    //     );

    //     if (!response) return;
    //     if (!wsUrl) return;

    //     if (!!store.lastWsConn && !!store.lastWsConn.close) {
    //       store.lastWsConn.close();
    //     }

    //     // Establish new WS connection if wsUrl provided
    //     const conn = await subscribeToLiveResource<T>(
    //       wsUrl,
    //       (msg) => {
    //         if (msg.resource === resourceType) {
    //           switch (msg.type) {
    //             case MessageType.UPDATE:
    //             case MessageType.INSERT:
    //               dispatch(this.update(msg.payload as any));
    //               break;
    //             case MessageType.DELETE:
    //               dispatch(this.delete(msg.payload as any));
    //               break;
    //             default:
    //               assertUnreachable(msg);
    //           }
    //         }
    //       },
    //       () => {},
    //       (_msg: string) => {
    //         if (conn && conn.close) {
    //           conn.close();
    //         }
    //       },
    //       wsFilter || filters
    //     );

    //     // Store last WS connection for later
    //     dispatch(this._logLastWsConn(conn));
    //   };
    // },
  };
}
