export interface RenderQueueItem {
  setup: (context: WebGL2RenderingContext) => void,
  render: (context: WebGL2RenderingContext) => void
}

export default class RenderQueue implements RenderQueueItem {
  private queue: RenderQueueItem[];
  private context?: WebGL2RenderingContext;

  constructor() {
    this.queue = [];


  }

  /**
   * Adds a RenderQueueItem to the queue, and sets it up if there already exists a rendering context.
   * @param item The item to add to the queue.
   */
  public push(item: RenderQueueItem): number {
    if (this.context !== undefined)
      item.setup(this.context);

    return this.queue.push(item) - 1;
  }

  /**
   * Requests all items in the render queue to perform all functionality to render.
   * @param context
   */
  public render(context: WebGL2RenderingContext): void {
    for (let i = 0; i < this.queue.length; i++) {
      this.queue[i].render(context);
    }
  }

  /**
   * Provides all items in the queue the context so that they can be set up for rendering
   * @param context The newly created rendering context
   */
  public setup(context: WebGL2RenderingContext): void {
    this.context = context;

    for (let i = 0; i < this.queue.length; i++) {
      this.queue[i].setup(context);
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

    this.queue[index].setup(this.context);
  }
}
