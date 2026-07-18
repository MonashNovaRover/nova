export interface LocalStorageState {
    map: {[key: string]: unknown},
}

export const initialLocalStorageState: LocalStorageState = {
    map: {},
}

export interface LSKeyValuePair {
    key: string,
    value: unknown,
}
