/*
 * Device Tree
 *
 * Copyright (C) 2012 Citrix Systems, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __XEN_DEVICE_TREE_H__
#define __XEN_DEVICE_TREE_H__

#include <asm/byteorder.h>
#include <public/xen.h>
#include <xen/init.h>
#include <xen/string.h>
#include <xen/types.h>

#define DEVICE_TREE_MAX_DEPTH 16

#define NR_MEM_BANKS 8

#define MOD_XEN    0
#define MOD_FDT    1
#define MOD_KERNEL 2
#define MOD_INITRD 3
#define NR_MODULES 4

#define MOD_DISCARD_FIRST MOD_FDT

struct membank {
    paddr_t start;
    paddr_t size;
};

struct dt_mem_info {
    int nr_banks;
    struct membank bank[NR_MEM_BANKS];
};

struct dt_mb_module {
    paddr_t start;
    paddr_t size;
    char cmdline[1024];
};

struct dt_module_info {
    int nr_mods;
    /* Module 0 is Xen itself, followed by the provided modules-proper */
    struct dt_mb_module module[NR_MODULES];
};

struct dt_early_info {
    struct dt_mem_info mem;
    struct dt_module_info modules;
};

/*
 * Struct used for matching a device
 */
struct dt_device_match {
    const char *path;
    const char *type;
    const char *compatible;
};

#define DT_MATCH_PATH(p)                { .path = p }
#define DT_MATCH_TYPE(typ)              { .type = typ }
#define DT_MATCH_COMPATIBLE(compat)     { .compatible = compat }

typedef u32 dt_phandle;

/**
 * dt_property - describe a property for a device
 * @name: name of the property
 * @length: size of the value
 * @value: pointer to data contained in the property
 * @next: pointer to the next property of a specific node
 */
struct dt_property {
    const char *name;
    u32 length;
    void *value;
    struct dt_property *next;
};

/**
 * dt_device_node - describe a node in the device tree
 * @name: name of the node
 * @type: type of the node (ie: memory, cpu, ...)
 * @full_name: full name, it's composed of all the ascendant name separate by /
 * @used_by: who owns the node? (ie: xen, dom0...)
 * @properties: list of properties for the node
 * @child: pointer to the first child
 * @sibling: pointer to the next sibling
 * @allnext: pointer to the next in list of all nodes
 */
struct dt_device_node {
    const char *name;
    const char *type;
    dt_phandle phandle;
    char *full_name;
    domid_t used_by; /* By default it's used by dom0 */

    struct dt_property *properties;
    struct dt_device_node *parent;
    struct dt_device_node *child;
    struct dt_device_node *sibling;
    struct dt_device_node *next; /* TODO: Remove it. Only use to know the last children */
    struct dt_device_node *allnext;

};

/**
 * IRQ line type.
 *
 * DT_IRQ_TYPE_NONE            - default, unspecified type
 * DT_IRQ_TYPE_EDGE_RISING     - rising edge triggered
 * DT_IRQ_TYPE_EDGE_FALLING    - falling edge triggered
 * DT_IRQ_TYPE_EDGE_BOTH       - rising and falling edge triggered
 * DT_IRQ_TYPE_LEVEL_HIGH      - high level triggered
 * DT_IRQ_TYPE_LEVEL_LOW       - low level triggered
 * DT_IRQ_TYPE_LEVEL_MASK      - Mask to filter out the level bits
 * DT_IRQ_TYPE_SENSE_MASK      - Mask for all the above bits
 */
#define DT_IRQ_TYPE_NONE           0x00000000
#define DT_IRQ_TYPE_EDGE_RISING    0x00000001
#define DT_IRQ_TYPE_EDGE_FALLING   0x00000002
#define DT_IRQ_TYPE_EDGE_BOTH                           \
    (DT_IRQ_TYPE_EDGE_FALLING | DT_IRQ_TYPE_EDGE_RISING)
#define DT_IRQ_TYPE_LEVEL_HIGH     0x00000004
#define DT_IRQ_TYPE_LEVEL_LOW      0x00000008
#define DT_IRQ_TYPE_LEVEL_MASK                          \
    (DT_IRQ_TYPE_LEVEL_LOW | DT_IRQ_TYPE_LEVEL_HIGH)
#define DT_IRQ_TYPE_SENSE_MASK     0x0000000f

/**
 * dt_irq - describe an IRQ in the device tree
 * @irq: IRQ number
 * @type: IRQ type (see DT_IRQ_TYPE_*)
 *
 * This structure is returned when an interrupt is mapped.
 */
struct dt_irq {
    unsigned int irq;
    unsigned int type;
};

/* If type == DT_IRQ_TYPE_NONE, assume we use level triggered */
static inline bool_t dt_irq_is_level_triggered(const struct dt_irq *irq)
{
    unsigned int type = irq->type;

    return (type & DT_IRQ_TYPE_LEVEL_MASK) || (type == DT_IRQ_TYPE_NONE);
}

/**
 * dt_raw_irq - container for device_node/irq_specifier for an irq controller
 * @controller: pointer to interrupt controller deivce tree node
 * @size: size of interrupt specifier
 * @specifier: array of cells @size long specifying the specific interrupt
 *
 * This structure is returned when an interrupt is mapped but not translated.
 */
#define DT_MAX_IRQ_SPEC     4 /* We handle specifiers of at most 4 cells */
struct dt_raw_irq {
    const struct dt_device_node *controller;
    u32 size;
    u32 specifier[DT_MAX_IRQ_SPEC];
};

#define dt_irq(irq) ((irq)->irq)
#define dt_irq_flags(irq) ((irq)->flags)

typedef int (*device_tree_node_func)(const void *fdt,
                                     int node, const char *name, int depth,
                                     u32 address_cells, u32 size_cells,
                                     void *data);

extern struct dt_early_info early_info;
extern const void *device_tree_flattened;

size_t __init device_tree_early_init(const void *fdt, paddr_t paddr);

const char __init *device_tree_bootargs(const void *fdt);
void __init device_tree_dump(const void *fdt);

/**
 * dt_unflatten_host_device_tree - Unflatten the host device tree
 *
 * Create a hierarchical device tree for the host DTB to be able
 * to retrieve parents.
 */
void __init dt_unflatten_host_device_tree(void);

/**
 * IRQ translation callback
 * TODO: For the moment we assume that we only have ONE
 * interrupt-controller.
 */
typedef int (*dt_irq_xlate_func)(const u32 *intspec, unsigned int intsize,
                                 unsigned int *out_hwirq,
                                 unsigned int *out_type);
extern dt_irq_xlate_func dt_irq_xlate;

/**
 * Host device tree
 * DO NOT modify it!
 */
extern struct dt_device_node *dt_host;

/**
 * Primary interrupt controller
 * Exynos SOC has an interrupt combiner, interrupt has no physical
 * meaning when it's not connected to the primary controller.
 * We will only map interrupt whose parent controller is
 * dt_interrupt_controller. It should always be a GIC.
 * TODO: Handle multiple GIC
 */
extern const struct dt_device_node *dt_interrupt_controller;

/**
 * Find the interrupt controller
 * For the moment we handle only one interrupt controller: the first
 * one without parent which is compatible with the string "compat".
 *
 * If found, return the interrupt controller device node.
 */
struct dt_device_node * __init
dt_find_interrupt_controller(const struct dt_device_match *matches);

#define dt_prop_cmp(s1, s2) strcmp((s1), (s2))
#define dt_node_cmp(s1, s2) strcasecmp((s1), (s2))
#define dt_compat_cmp(s1, s2) strcasecmp((s1), (s2))

/* Default #address and #size cells */
#define DT_ROOT_NODE_ADDR_CELLS_DEFAULT 2
#define DT_ROOT_NODE_SIZE_CELLS_DEFAULT 1

#define dt_for_each_property_node(dn, pp)                   \
    for ( pp = dn->properties; pp != NULL; pp = pp->next )

#define dt_for_each_device_node(dt, dn)                     \
    for ( dn = dt; dn != NULL; dn = dn->allnext )

#define dt_for_each_child_node(dt, dn)                      \
    for ( dn = dt->child; dn != NULL; dn = dn->sibling )

/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 dt_read_number(const __be32 *cell, int size)
{
    u64 r = 0;

    while ( size-- )
        r = (r << 32) | be32_to_cpu(*(cell++));
    return r;
}

/* Helper to convert a number of cells to bytes */
static inline int dt_cells_to_size(int size)
{
    return (size * sizeof (u32));
}

/* Helper to convert a number of bytes to cells, rounds down */
static inline int dt_size_to_cells(int bytes)
{
    return (bytes / sizeof(u32));
}

static inline u64 dt_next_cell(int s, const __be32 **cellp)
{
    const __be32 *p = *cellp;

    *cellp = p + s;
    return dt_read_number(p, s);
}

static inline const char *dt_node_full_name(const struct dt_device_node *np)
{
    return (np && np->full_name) ? np->full_name : "<no-node>";
}

static inline const char *dt_node_name(const struct dt_device_node *np)
{
    return (np && np->name) ? np->name : "<no-node>";
}

static inline bool_t dt_node_name_is_equal(const struct dt_device_node *np,
                                           const char *name)
{
    return !dt_node_cmp(np->name, name);
}

static inline bool_t dt_node_path_is_equal(const struct dt_device_node *np,
                                           const char *path)
{
    return !dt_node_cmp(np->full_name, path);
}

static inline bool_t
dt_device_type_is_equal(const struct dt_device_node *device,
                        const char *type)
{
    return !dt_node_cmp(device->type, type);
}

static inline void dt_device_set_used_by(struct dt_device_node *device,
                                         domid_t used_by)
{
    /* TODO: children must inherit to the used_by thing */
    device->used_by = used_by;
}

static inline domid_t dt_device_used_by(const struct dt_device_node *device)
{
    return device->used_by;
}

static inline bool_t dt_property_name_is_equal(const struct dt_property *pp,
                                               const char *name)
{
    return !dt_prop_cmp(pp->name, name);
}

/**
 * dt_find_compatible_node - Find a node based on type and one of the
 *                           tokens in its "compatible" property
 * @from: The node to start searching from or NULL, the node
 *          you pass will not be searched, only the next one
 *          will; typically, you pass what the previous call
 *          returned.
 * @type: The type string to match "device_type" or NULL to ignore
 * @compatible: The string to match to one of the tokens in the device
 *          "compatible" list.
 *
 * Returns a node pointer.
 */
struct dt_device_node *dt_find_compatible_node(struct dt_device_node *from,
                                               const char *type,
                                               const char *compatible);

/**
 * Find a property with a given name for a given node
 * and return the value.
 */
const void *dt_get_property(const struct dt_device_node *np,
                            const char *name, u32 *lenp);

/**
 * dt_property_read_u32 - Helper to read a u32 property.
 * @np: node to get the value
 * @name: name of the property
 * @out_value: pointer to return value
 *
 * Return true if get the desired value.
 */
bool_t dt_property_read_u32(const struct dt_device_node *np,
                            const char *name, u32 *out_value);
/**
 * dt_property_read_u64 - Helper to read a u64 property.
 * @np: node to get the value
 * @name: name of the property
 * @out_value: pointer to return value
 *
 * Return true if get the desired value.
 */
bool_t dt_property_read_u64(const struct dt_device_node *np,
                            const char *name, u64 *out_value);

/**
 * dt_property_read_string - Find and read a string from a property
 * @np:         Device node from which the property value is to be read
 * @propname:   Name of the property to be searched
 * @out_string: Pointer to null terminated return string, modified only
 *              if return value if 0.
 *
 * Search for a property in a device tree node and retrieve a null
 * terminated string value (pointer to data, not a copy). Returns 0 on
 * success, -EINVAL if the property does not exist, -ENODATA if property
 * doest not have value, and -EILSEQ if the string is not
 * null-terminated with the length of the property data.
 *
 * The out_string pointer is modified only if a valid string can be decoded.
 */
int dt_property_read_string(const struct dt_device_node *np,
                            const char *propname, const char **out_string);

/**
 * Checks if the given "compat" string matches one of the strings in
 * the device's "compatible" property
 */
bool_t dt_device_is_compatible(const struct dt_device_node *device,
                               const char *compat);

/**
 * dt_machine_is_compatible - Test root of device tree for a given compatible value
 * @compat: compatible string to look for in root node's compatible property.
 *
 * Returns true if the root node has the given value in its
 * compatible property.
 */
bool_t dt_machine_is_compatible(const char *compat);

/**
 * dt_find_node_by_name - Find a node by its "name" property
 * @from: The node to start searching from or NULL, the node
 * you pass will not be searched, only the next one
 *  will; typically, you pass what the previous call
 *  returned. of_node_put() will be called on it
 * @name: The name string to match against
 *
 * Returns a node pointer with refcount incremented, use
 * of_node_put() on it when done.
 */
struct dt_device_node *dt_find_node_by_name(struct dt_device_node *node,
                                            const char *name);

/**
 * dt_find_node_by_type - Find a node by its "type" property
 */
struct dt_device_node *dt_find_node_by_type(struct dt_device_node *from,
                                            const char *type);

/**
 * df_find_node_by_alias - Find a node matching an alias
 * @alias: The alias to match
 *
 * Returns a node pointer.
 */
struct dt_device_node *dt_find_node_by_alias(const char *alias);

/**
 * dt_find_node_by_path - Find a node matching a full DT path
 * @path: The full path to match
 *
 * Returns a node pointer.
 */
struct dt_device_node *dt_find_node_by_path(const char *path);

/**
 * dt_get_parent - Get a node's parent if any
 * @node: Node to get parent
 *
 * Returns a node pointer.
 */
const struct dt_device_node *dt_get_parent(const struct dt_device_node *node);

/**
 * dt_device_get_address - Resolve an address for a device
 * @device: the device whose address is to be resolved
 * @index: index of the address to resolve
 * @addr: address filled by this function
 * @size: size filled by this function
 *
 * This function resolves an address, walking the tree, for a give
 * device-tree node. It returns 0 on success.
 */
int dt_device_get_address(const struct dt_device_node *dev, int index,
                          u64 *addr, u64 *size);

/**
 * dt_number_of_irq - Get the number of IRQ for a device
 * @device: the device whose number of interrupt is to be retrieved
 *
 * Return the number of irq for this device or 0 if there is no
 * interrupt or an error occurred.
 */
unsigned int dt_number_of_irq(const struct dt_device_node *device);

/**
 * dt_number_of_address - Get the number of addresses for a device
 * @device: the device whose number of address is to be retrieved
 *
 * Return the number of address for this device or 0 if there is no
 * address or an error occurred.
 */
unsigned int dt_number_of_address(const struct dt_device_node *device);

/**
 * dt_device_get_irq - Resolve an interrupt for a device
 * @device: the device whose interrupt is to be resolved
 * @index: index of the interrupt to resolve
 * @out_irq: structure dt_irq filled by this function
 *
 * This function resolves an interrupt, walking the tree, for a given
 * device-tree node. It's the high level pendant to dt_device_get_raw_irq().
 */
int dt_device_get_irq(const struct dt_device_node *device, int index,
                      struct dt_irq *irq);

/**
 * dt_device_get_raw_irq - Resolve an interrupt for a device without translation
 * @device: the device whose interrupt is to be resolved
 * @index: index of the interrupt to resolve
 * @out_irq: structure dt_raw_irq filled by this function
 *
 * This function resolves an interrupt for a device, no translation is
 * made. dt_irq_translate can be called after.
 */
int dt_device_get_raw_irq(const struct dt_device_node *device, int index,
                          struct dt_raw_irq *irq);

/**
 * dt_irq_translate - Translate an irq
 * @raw: IRQ to translate (raw format)
 * @out_irq: structure dt_irq filled by this function
 */
int dt_irq_translate(const struct dt_raw_irq *raw, struct dt_irq *out_irq);

/**
 * dt_n_size_cells - Helper to retrieve the number of cell for the size
 * @np: node to get the value
 *
 * This function retrieves for a give device-tree node the number of
 * cell for the size field.
 */
int dt_n_size_cells(const struct dt_device_node *np);

/**
 * dt_n_addr_cells - Helper to retrieve the number of cell for the address
 * @np: node to get the value
 *
 * This function retrieves for a give device-tree node the number of
 * cell for the address field.
 */
int dt_n_addr_cells(const struct dt_device_node *np);

/**
 * dt_device_is_available - Check if a device is available for use
 *
 * @device: Node to check for availability
 *
 * Returns true if the status property is absent or set to "okay" or "ok",
 * false otherwise.
 */
bool_t dt_device_is_available(const struct dt_device_node *device);

/**
 * dt_match_node - Tell if a device_node has a matching of dt_device_match
 * @matches: array of dt_device_match structures to search in
 * @node: the dt_device_node structure to match against
 *
 * Returns true if the device node match one of dt_device_match.
 */
bool_t dt_match_node(const struct dt_device_match *matches,
                     const struct dt_device_node *node);

/**
 * dt_find_matching_node - Find a node based on an dt_device_match match table
 * @from: The node to start searching from or NULL, the node you pass
 *        will not be searched, only the next one will; typically, you pass
 *        what the returned call returned
 * @matches: array of dt_device_match structures to search in
 *
 * Returns a node pointer.
 */
struct dt_device_node *
dt_find_matching_node(struct dt_device_node *from,
                      const struct dt_device_match *matches);

/**
 * dt_set_cell - Write a value into a series of cells
 *
 * @cellp: Pointer to cells
 * @size: number of cells to write the value
 * @value: number to write
 *
 * Write a value into a series of cells and update cellp to point to the
 * cell just after.
 */
void dt_set_cell(__be32 **cellp, int size, u64 val);

/**
 * dt_set_range - Write range into a series of cells
 *
 * @cellp: Pointer to cells
 * @np: Node which contains the encoding for the address and the size
 * @address: Start of range
 * @size: Size of the range
 *
 * Write a range into a series of cells and update cellp to point to the
 * cell just after.
 */
void dt_set_range(__be32 **cellp, const struct dt_device_node *np,
                  u64 address, u64 size);

/**
 * dt_get_range - Read a range (address/size) from a series of cells
 *
 * @cellp: Pointer to cells
 * @np Node which  contains the encoding for the addresss and the size
 * @address: Address filled by this function
 * @size: Size filled by this function
 *
 * WARNING: This function should not be used to decode an address
 * This function reads a range (address/size) from a series of cells and
 * update cellp to point to the cell just after.
 */
void dt_get_range(const __be32 **cellp, const struct dt_device_node *np,
                  u64 *address, u64 *size);

#endif /* __XEN_DEVICE_TREE_H */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */