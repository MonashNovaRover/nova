export interface RenderQueueItem<T extends unknown[]> {
  setup: (...args: T) => void,
  render: (...args: T) => void
}

/**
 * This is the data structure used to set up and sequence rendering.
 *
 * Each item is called in the order that they pushed to the queue when rendering, allowing for the order of useProgram
 * hooks (or other future implementations) to be the order that they are rendered in.
 *
 * To set up programs, we need a WebGL2RenderingContext. To get a context, we need a canvas in the dom. UseGL gets this
 * from a useRef, which is null on first render. So, we cannot guarantee that it exists. Queue items include a setup
 * function, which allows them to be constructed without the WebGL2RenderingContext, and be registered to run some setup
 * using the rendering context when it becomes available.
 */
export default class RenderQueue<T extends unknown[]> implements RenderQueueItem<T> {
  private queue: RenderQueueItem<T>[];
  private context?: T;

  constructor() {
    this.queue = [];
  }

  /**
   * Adds a RenderQueueItem to the queue, and sets it up if there already exists a rendering context.
   * @param item The item to add to the queue.
   */
  public push(item: RenderQueueItem<T>): number {
    if (this.context !== undefined)
      item.setup(...this.context);

    return this.queue.push(item) - 1;
  }

  /**
   * Requests all items in the render queue to perform all functionality to render.
   * @param context
   */
  public render(...context: T): void {
    for (let i = 0; i < this.queue.length; i++) {
      this.queue[i].render(...context);
    }
  }

  /**
   * Provides all items in the queue the context so that they can be set up for rendering.
   * @param context The newly created rendering context
   */
  public setup(...context: T): void {
    this.context = context;

    for (let i = 0; i < this.queue.length; i++) {
      this.queue[i].setup(...context);
    }
  }

  /**
   * Requests that setup be called for some existing item in the queue.
   * @param index The index returned by this.push to identify the item in the queue.
   */
  public update(index: number) {
    if (this.context === undefined)
      return;

    if (index > this.queue.length || index < 0)
      return;

    this.queue[index].setup(...this.context);
  }
}
