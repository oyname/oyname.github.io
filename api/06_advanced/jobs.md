# Job System

`engine::jobs::JobSystem`

A thread pool for fire-and-forget tasks, result-returning tasks, and parallel batch processing. Worker count defaults to `hardware_concurrency - 1`. All methods are thread-safe after `Initialize`.

---

## Setup

```cpp
jobs::JobSystem jobs;
jobs.Initialize();          // hardware_concurrency - 1 workers
// or:
jobs.Initialize(4u);        // explicit worker count
```

`Shutdown` is called automatically by the destructor.

```cpp
jobs.WorkerCount()  // number of active worker threads
jobs.IsParallel()   // false when WorkerCount == 0 (single-threaded fallback)
```

---

## Fire and forget

```cpp
jobs.Dispatch([]() {
    BuildNavMesh();
});
```

---

## Task with result

```cpp
std::future<jobs::TaskResult> future = jobs.DispatchResult([]() -> jobs::TaskResult {
    if (!LoadLevel())
        return jobs::TaskResult::Fail("level load failed");
    return jobs::TaskResult::Ok();
});

// later:
jobs::TaskResult result = future.get();
if (result.Failed())
    Debug::LogError(result.errorMessage);
```

---

## Task with return value

```cpp
std::future<jobs::ValueResult<Mesh>> future = jobs.DispatchReturn([]() -> Mesh {
    return ParseOBJ("cube.obj");
});

auto result = future.get();
if (result.Succeeded())
    Use(result.value.value());
```

---

## Parallel batch processing

Splits `itemCount` items across workers. Each worker receives a `[begin, end)` range.

```cpp
jobs::TaskResult result = jobs.ParallelFor(
    entities.size(),
    [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i)
            UpdateEntity(entities[i]);
    },
    /*minBatchSize=*/ 64u
);
```

The lambda may optionally return `TaskResult`:

```cpp
jobs.ParallelFor(count, [&](size_t begin, size_t end) -> jobs::TaskResult {
    for (size_t i = begin; i < end; ++i)
        if (!Process(i))
            return jobs::TaskResult::Fail("process failed");
    return jobs::TaskResult::Ok();
});
```

`minBatchSize` sets the minimum number of items per worker batch. Lower values increase parallelism but add scheduling overhead — 64 is a reasonable default.

---

## Waiting for all work to finish

```cpp
jobs.WaitIdle();
```

Blocks the calling thread until all queued tasks have completed. Use at shutdown or at explicit synchronization points (e.g. end of loading phase).

---

## TaskResult

```cpp
jobs::TaskResult::Ok()              // success
jobs::TaskResult::Fail("message")   // failure with description

result.Succeeded()
result.Failed()
result.errorMessage                 // nullptr on success
```

---

## Notes

- `JobSystem` is non-copyable.
- Tasks must not capture references to stack variables that may go out of scope before the task completes.
- `ParallelFor` blocks until all batches finish — it is synchronous from the caller's perspective.
- When `WorkerCount` is 0, `Dispatch` runs the task immediately on the calling thread. This makes single-threaded debugging straightforward.
