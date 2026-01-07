import {useEffect} from "react";

// used to avoid repeated code, were some enum mapped to a value in a webgl 2 context.
export default class MappedGLint<T> {
  private readonly mappingFunction: (context: WebGL2RenderingContext, value: T) => GLint;

  private _value: T;
  private _mappedValue?: GLint;

  constructor(mappingFunction: (context: WebGL2RenderingContext, value: T) => GLint, initialValue: T) {
    this.mappingFunction = mappingFunction;
    this._value = initialValue;
  }

  set value(newValue: T) {
    this._value = newValue;
    this._mappedValue = undefined;
  }

  // You should call this.validate first
  get value() : GLint {
    return this._mappedValue ?? -1;
  }

  validate(context: WebGL2RenderingContext) {
    if (this._mappedValue)
      return;

    this._mappedValue = this.mappingFunction(context, this._value);
  }
}

export function useMappedGLint<T>(mappedGLint: MappedGLint<T>, value?: T) {
  useEffect(() => {
    if (value)
      mappedGLint.value = value;
  }, [mappedGLint, value]);
}