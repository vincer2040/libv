#ifndef __LIBV_LIST_H__

#define __LIBV_LIST_H__

#include "libv/base/base.h"
#include <stdbool.h>

LIBV_BEGIN

typedef struct {
    size_t size;
    size_t align;
    void (*copy)(void* dst, const void* src);
    bool (*eq)(const void* needle, const void* candidate);
    void (*dtor)(void* value);
} list_object_policy;

typedef struct {
    const libv_basic_alloc_policy* alloc;
    const list_object_policy* object;
} list_policy;

#define LIST_DECLARE_DEFAULT_ALLOC_POLICY(name_)                               \
    const libv_basic_alloc_policy name_##_default_alloc_policy = {             \
        .alloc = libv_default_alloc,                                           \
        .free = libv_default_free,                                             \
    }

#define LIST_DECLARE_DEFAULT_OBJECT_POLICY(name_, type_)                       \
    void name_##_default_object_copy(void* dst, const void* src) {             \
        memcpy(dst, src, sizeof(type_));                                       \
    }                                                                          \
    bool name_##_default_object_eq(const void* needle,                         \
                                   const void* candidate) {                    \
        return memcmp(needle, candidate, sizeof(type_)) == 0;                  \
    }                                                                          \
    list_object_policy name_##_default_object_policy = {                       \
        .size = sizeof(type_),                                                 \
        .align = _Alignof(type_),                                              \
        .copy = name_##_default_object_copy,                                   \
        .eq = name_##_default_object_eq,                                       \
        .dtor = NULL,                                                          \
    }

#define LIST_DECLARE_DEFAULT_POLICY(name_, type_)                              \
    LIST_DECLARE_DEFAULT_ALLOC_POLICY(name_);                                  \
    LIST_DECLARE_DEFAULT_OBJECT_POLICY(name_, type_);                          \
    const list_policy name_##_policy = {                                       \
        .alloc = &name_##_default_alloc_policy,                                \
        .object = &name_##_default_object_policy,                              \
    }

typedef struct list_node list_node;

struct list_node {
    list_node* previous;
    list_node* next;
    unsigned char data[];
};

static inline list_node* list_node_new(const list_policy* policy,
                                       const void* data) {
    list_node* self = policy->alloc->alloc(sizeof *self + policy->object->size,
                                           _Alignof(list_node));
    self->previous = self->next = NULL;
    policy->object->copy(self->data, data);
    return self;
}

static inline void list_node_destroy(const list_policy* policy,
                                     list_node* self) {
    if (policy->object->dtor) {
        policy->object->dtor(self->data);
    }
    policy->alloc->free(self, sizeof *self + policy->object->size,
                        _Alignof(list_node));
}

typedef struct {
    list_node* head;
    list_node* tail;
    size_t size;
} list_raw;

static inline list_raw list_raw_new(void) { return (list_raw){0}; }

static inline void list_raw_destroy(const list_policy* policy, list_raw* self) {
    list_node* current = self->head;
    while (true) {
        if (!current) {
            break;
        }
        list_node* next = current->next;
        list_node_destroy(policy, current);
        current = next;
    }
    self->head = self->tail = NULL;
    self->size = 0;
}

static inline size_t list_raw_size(const list_raw* self) { return self->size; }

static inline bool list_raw_maybe_insert_at_head(list_raw* self,
                                                 list_node* new_node) {
    if (self->head == NULL) {
        libv_assert(self->tail == NULL,
                    "list in invalid state, head == NULL but tail != NULL");
        self->head = self->tail = new_node;
        ++self->size;
        return true;
    }
    return false;
}

static inline void list_raw_push_back(const list_policy* policy, list_raw* self,
                                      const void* data) {
    list_node* new_node = list_node_new(policy, data);

    if (list_raw_maybe_insert_at_head(self, new_node)) {
        return;
    }

    self->tail->next = new_node;
    new_node->previous = self->tail;
    self->tail = new_node;
    ++self->size;
}

static inline void list_raw_push_front(const list_policy* policy,
                                       list_raw* self, const void* data) {
    list_node* new_node = list_node_new(policy, data);

    if (list_raw_maybe_insert_at_head(self, new_node)) {
        return;
    }

    self->head->previous = new_node;
    new_node->next = self->head;
    self->head = new_node;
    ++self->size;
}

static inline list_node* list_raw_get_node_at_index(const list_raw* self,
                                                    size_t index) {
    if (index >= self->size) {
        return NULL;
    }
    list_node* current = self->head;
    for (size_t i = 0; i < index; ++i) {
        current = current->next;
    }
    return current;
}

static inline int list_raw_insert_at_index(const list_policy* policy,
                                           list_raw* self, const void* data,
                                           size_t index) {
    list_node* new_node;
    if (index == 0 && self->size == 0) {
        new_node = list_node_new(policy, data);
        return list_raw_maybe_insert_at_head(self, new_node) - 1;
    }
    list_node* node_at_index = list_raw_get_node_at_index(self, index);
    if (node_at_index == NULL) {
        return LIBV_ERR;
    }

    new_node = list_node_new(policy, data);

    new_node->next = node_at_index;
    new_node->previous = node_at_index->previous;

    if (node_at_index->previous) {
        node_at_index->previous->next = new_node;
    } else {
        // inserting at head of non-empty list
        self->head = new_node;
    }

    node_at_index->previous = new_node;

    ++self->size;
    return LIBV_OK;
}

static inline int list_raw_set_at_index(const list_policy* policy,
                                        list_raw* self, const void* data,
                                        size_t index) {
    list_node* node_at_index = list_raw_get_node_at_index(self, index);
    if (node_at_index == NULL) {
        return LIBV_ERR;
    }
    if (policy->object->dtor) {
        policy->object->dtor(node_at_index->data);
    }
    policy->object->copy(node_at_index->data, data);
    return LIBV_OK;
}

static inline int list_raw_remove_at_index(const list_policy* policy,
                                           list_raw* self, size_t index) {
    list_node* node_at_index = list_raw_get_node_at_index(self, index);
    if (node_at_index == NULL) {
        return LIBV_ERR;
    }
    node_at_index->previous->next = node_at_index->next;
    node_at_index->next->previous = node_at_index->previous;
    list_node_destroy(policy, node_at_index);
    --self->size;
    if (self->size == 0) {
        self->head = self->tail = NULL;
    }
    return LIBV_OK;
}

static inline int list_raw_pop_back(const list_policy* policy, list_raw* self,
                                    void* out) {
    if (self->size == 0) {
        return LIBV_ERR;
    }
    list_node* node = self->tail;
    if (out) {
        policy->object->copy(out, node->data);
    }
    --self->size;
    if (self->size == 0) {
        self->head = self->tail = NULL;
    } else {
        self->tail = node->previous;
        self->tail->next = NULL;
    }
    list_node_destroy(policy, node);
    return LIBV_OK;
}

static inline int list_raw_pop_front(const list_policy* policy, list_raw* self,
                                     void* out) {
    if (self->size == 0) {
        return LIBV_ERR;
    }
    list_node* node = self->head;
    if (out) {
        policy->object->copy(out, node->data);
    }
    --self->size;
    if (self->size == 0) {
        self->head = self->tail = NULL;
    } else {
        self->head = node->next;
        self->head->previous = NULL;
    }
    list_node_destroy(policy, node);
    return LIBV_OK;
}

static inline const void* list_raw_front(const list_raw* self) {
    return self->head ? self->head->data : NULL;
}

static inline const void* list_raw_back(const list_raw* self) {
    return self->tail ? self->tail->data : NULL;
}

static inline const void* list_raw_get_at_index(const list_raw* self,
                                                size_t index) {
    list_node* found = list_raw_get_node_at_index(self, index);
    return found ? found->data : NULL;
}

static inline bool list_raw_contains(const list_policy* policy,
                                     const list_raw* self, const void* data) {
    const list_node* current = self->head;
    for (size_t i = 0; i < self->size; ++i) {
        if (policy->object->eq(current->data, data)) {
            return true;
        }
        current = current->next;
    }
    return false;
}

#define LIST_DECLARE(name_, policy_, type_)                                    \
    LIBV_BEGIN                                                                 \
    typedef struct {                                                           \
        list_raw list;                                                         \
    } name_;                                                                   \
    static inline name_ name_##_new(void) { return (name_){list_raw_new()}; }  \
    static inline void name_##_destroy(name_* self) {                          \
        list_raw_destroy(&policy_, &self->list);                               \
    }                                                                          \
    static inline size_t name_##_size(const name_* self) {                     \
        return list_raw_size(&self->list);                                     \
    }                                                                          \
    static inline void name_##_push_back(name_* self, const type_* data) {     \
        list_raw_push_back(&policy_, &self->list, data);                       \
    }                                                                          \
    static inline void name_##_push_front(name_* self, const type_* data) {    \
        list_raw_push_front(&policy_, &self->list, data);                      \
    }                                                                          \
    static inline int name_##_insert_at_index(name_* self, const type_* data,  \
                                              size_t index) {                  \
        return list_raw_insert_at_index(&policy_, &self->list, data, index);   \
    }                                                                          \
    static inline int name_##_set_at_index(name_* self, const type_* data,     \
                                           size_t index) {                     \
        return list_raw_set_at_index(&policy_, &self->list, data, index);      \
    }                                                                          \
    static inline int name_##_remove_at_index(name_* self, size_t index) {     \
        return list_raw_remove_at_index(&policy_, &self->list, index);         \
    }                                                                          \
    static inline int name_##_pop_back(name_* self, type_* out) {              \
        return list_raw_pop_back(&policy_, &self->list, out);                  \
    }                                                                          \
    static inline int name_##_pop_front(name_* self, type_* out) {             \
        return list_raw_pop_front(&policy_, &self->list, out);                 \
    }                                                                          \
    static inline const type_* name_##_front(const name_* self) {              \
        return (const type_*)list_raw_front(&self->list);                      \
    }                                                                          \
    static inline const type_* name_##_back(const name_* self) {               \
        return (const type_*)list_raw_back(&self->list);                       \
    }                                                                          \
    static inline const type_* name_##_get_at_index(const name_* self,         \
                                                    size_t index) {            \
        return (const type_*)list_raw_get_at_index(&self->list, index);        \
    }                                                                          \
    static inline bool name_##_contains(const name_* self,                     \
                                        const type_* data) {                   \
        return list_raw_contains(&policy_, &self->list, data);                 \
    }                                                                          \
    LIBV_END                                                                   \
    /* Force a semicolon. */ struct name_##_needstrailingsemicolon_ { int x; }

#define LIST_DECLARE_DEFAULT(name_, type_)                                     \
    LIST_DECLARE_DEFAULT_POLICY(name_, type_);                                 \
    LIST_DECLARE(name_, name_##_policy, type_)

LIBV_END

#endif // __LIBV_LIST_H__
