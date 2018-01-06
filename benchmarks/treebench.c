/*
    treebench

    Written by Scott Robert Ladd (scott@coyotegulch.com)
    No rights reserved. This is public domain software, for use by anyone.

    A tree-generating benchmark that can be used as a fitness test for
    evolving optimal compiler options via genetic algorithm.

    Note that the code herein is design for the purpose of testing 
    computational performance; error handling and other such "niceties"
    is virtually non-existent. Also, the code is not necessarily hand-
    optimized.

    Actual benchmark results can be found at:
            http://www.coyotegulch.com

    Please do not use this information or algorithm in any way that might
    upset the balance of the universe or otherwise cause a massive
    deforestation.
*/

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static const int TEST_SIZE =  10000000;
static const int ORDER     =        16;
static const int MAX_KEY   =      4096;

// type definitions
typedef int32_t btree_count_t; // counter type
typedef int32_t btree_key_t;   // key type
typedef int32_t btree_data_t;  // data type

// constants
static const btree_key_t NULL_KEY  = -1;
static const btree_key_t NULL_DATA = -1;

// structures
#if defined(__GNUC__)
#pragma pack(1)
#endif

typedef struct btree_page_s
{
    // header information
    struct btree_page_s * m_parent;  // parent in page file
    uint16_t       m_key_count;      // actual  # of keys
    
    // element arrays
    btree_key_t *  m_keys;           // array of key values
    btree_data_t * m_data;           // reference pointers
    struct btree_page_s **  m_links; // links to other pages
}
btree_page;

typedef struct
{
    // header information    
    uint16_t      m_order;      // number of keys per page  
    uint16_t      m_links_size; // number of links per page; order + 1; calculated at creation time
    uint16_t      m_min_keys;   // minimum # of keys; order / 2; calculated at creation time
    btree_count_t m_count;      // counts number of active records
    btree_count_t m_ticker;     // counts the total number of new records added
    btree_page  * m_root;       // pointer to location of root page
}
btree;

typedef struct
{
    btree_page * m_page;
    int          m_index;
    bool         m_found;
} search_result;

#if defined(__GNUC__)
#pragma pack()
#endif

static btree_page * alloc_page(const btree * btree)
{
    // catch errors
    bool error = false;
    
    // allocate page
    btree_page * page = (btree_page *)malloc(sizeof(btree_page));
    
    if (page != NULL)
    {
        // fill header with default values
        page->m_parent    = NULL;
        page->m_key_count = 0;
            
        // allocate key array
        page->m_keys = (btree_key_t *)malloc(sizeof(btree_key_t) * btree->m_order);

        if (page->m_keys != NULL)
        {
            // allocate data array
            page->m_data = (btree_data_t *)malloc(sizeof(btree_data_t) * btree->m_order);
            
            if (page->m_data != NULL)
            {
                // allocate data array
                page->m_links = (btree_page **)malloc(sizeof(btree_page *) * btree->m_order);
            
                if (page->m_links == NULL)
                {
                    free(page->m_data);
                    free(page->m_keys);
                    free(page);
                    page = NULL;
                    error = true;
                }
            }
            else
            {
                free(page->m_keys);
                free(page);
                page = NULL;
                error = true;
            }
        }
        else
        {
            free(page);
            page = NULL;
            error = true;
        }
    }
    else
    {
        error = true;
    }

    if (!error)
    {
        // initialize
        for (int n = 0; n < btree->m_order; ++n)
        {
            page->m_keys[n]  = NULL_KEY;
            page->m_data[n]  = NULL_DATA;
            page->m_links[n] = NULL;
        }

        page->m_links[btree->m_order] = NULL;
    }
    
    // done
    return page;
}

static void free_page(btree_page * page)
{
    // clean up
    free(page->m_links);
    free(page->m_data);
    free(page->m_keys);
    free(page);
}

static btree * create_btree(uint16_t order)
{
    // what we return
    btree * tree = (btree *)malloc(sizeof(btree));
    
    // make sure the arguments make sense
    if ((tree != NULL) && (order > 2))
    {
        // save order; calculate values derived from order
        tree->m_order      = order;
        tree->m_links_size = order + 1;
        tree->m_min_keys   = order / 2;

        // set counts and tickers
        tree->m_count      = 0;
        tree->m_ticker     = 0;

        // create root page
        tree->m_root = alloc_page(tree);
        
        if (tree->m_root == NULL)
        {
            free(tree);
            tree = NULL;
        }
    }
    
    // done    
    return tree;
}

static void recursive_destroy_page(btree_page * page)
{
    for (int n = 0; n <= page->m_key_count; ++n)
    {
        if (page->m_links[n] != NULL)
            recursive_destroy_page(page->m_links[n]);
        
    }
    free(page);
}

static void free_btree(btree * btree)
{
    // make sure the arguments make sense
    if (btree != NULL)
    {
        recursive_destroy_page(btree->m_root);
        free(btree);
    }
}

// search for leaf node in which we insert the given key    
static search_result search(btree * btree, btree_page * page, btree_key_t key)
{
    // what we return
    search_result result;
    
    int index = 0;

    // if page is empty, we didn't find the key
    if (page->m_key_count == 0)
    {
        result.m_page  = page;
        result.m_index = index;
        result.m_found = false;
    }
    else
    {
        // loop through keys
        while (index < page->m_key_count)
        {
            // compare keys
            if (page->m_keys[index] < key)
                ++index;
            else
            {
                if (page->m_keys[index] == key)
                {
                    result.m_page  = page;
                    result.m_index = index;
                    result.m_found = true;
                    return result;
                }
                
                break;
            }
        }

        // if we're in a leaf, the key hasn't been found
        if (page->m_links[index] == NULL)
        {
            result.m_page  = page;
            result.m_index = index;
            result.m_found = false;
        }
        else
        {
            // read and search next page
            btree_page * next_page = page->m_links[index];
            
            // search next page
            result = search(btree,next_page,key);
        }
    }
    
    // done
    return result;
}

// promote key by creating new root
static void promote_root(btree * btree,
                         btree_key_t   key,
                         btree_data_t  datum,
                         btree_page * page_before,
                         btree_page * page_after)
{
    // create a new root page
    btree_page * new_root = alloc_page(btree);

    // add key and links to root
    new_root->m_keys[0]   = key;
    new_root->m_data[0]   = datum;
    new_root->m_links[0]  = page_before;
    new_root->m_links[1]  = page_after;
    new_root->m_key_count = 1;

    // assign new root to tree
    btree->m_root = new_root;

    // update children
    page_before->m_parent = new_root;
    page_after->m_parent  = new_root;
}

// promote key into parent
static void promote_internal(btree * btree,
                             btree_page * page_insert,
                             btree_key_t  key,
                             btree_data_t datum,
                             btree_page * link)
{
    if (page_insert->m_key_count == btree->m_order)
    {
        // temporary array
        btree_key_t * temp_keys        = (btree_key_t *)malloc(sizeof(btree_key_t) * (btree->m_order + 1));
        btree_data_t * temp_data       = (btree_data_t *)malloc(sizeof(btree_data_t) * (btree->m_order + 1));
        btree_page ** temp_links = (btree_page **)malloc(sizeof(btree_page *) * (btree->m_order + 2));

        temp_links[0] = page_insert->m_links[0];

        // copy entries from inapage to temps
        int nt  = 0;
        int ni  = 0;
        int insert_index = 0;

        // find insertion point
        while ((insert_index < page_insert->m_key_count) && (page_insert->m_keys[insert_index] < key))
            ++insert_index;

        // store new info
        temp_keys[insert_index]      = key;
        temp_data[insert_index]      = datum;
        temp_links[insert_index + 1] = link;

        // copy existing keys
        while (ni < btree->m_order)
        {
            if (ni == insert_index)
                ++nt;

            temp_keys[nt]      = page_insert->m_keys[ni];
            temp_data[nt]      = page_insert->m_data[ni];
            temp_links[nt + 1] = page_insert->m_links[ni + 1];

            ++ni;
            ++nt;
        }

        // generate a new leaf node
        btree_page * page_sibling = alloc_page(btree);
        page_sibling->m_parent = page_insert->m_parent;

        // clear key counts
        page_insert->m_key_count  = 0;
        page_sibling->m_key_count = 0;

        page_insert->m_links[0] = temp_links[0];

        // copy keys from temp to pages
        for (ni = 0; ni < btree->m_min_keys; ++ni)
        {
            page_insert->m_keys[ni]      = temp_keys[ni];
            page_insert->m_data[ni]      = temp_data[ni];
            page_insert->m_links[ni + 1] = temp_links[ni + 1];

            ++page_insert->m_key_count;
        }

        page_sibling->m_links[0] = temp_links[btree->m_min_keys + 1];

        for (ni = btree->m_min_keys + 1; ni <= btree->m_order; ++ni)
        {
            page_sibling->m_keys[ni - 1 - btree->m_min_keys] = temp_keys[ni];
            page_sibling->m_data[ni - 1 - btree->m_min_keys] = temp_data[ni];
            page_sibling->m_links[ni - btree->m_min_keys]    = temp_links[ni + 1];

            ++page_sibling->m_key_count;
        }

        // replace unused entries with null
        for (ni = btree->m_min_keys; ni < btree->m_order; ++ni)
        {
            page_insert->m_keys[ni]      = NULL_KEY;
            page_insert->m_data[ni]      = NULL_DATA;
            page_insert->m_links[ni + 1] = NULL;
        }

        // update parent links in child nodes
        for (ni = 0; ni <= page_sibling->m_key_count; ++ni)
        {
            btree_page * child = page_sibling->m_links[ni];
            child->m_parent = page_sibling;
        }

        // promote key and pointer
        if (page_insert->m_parent == NULL)
        {
            // create a new root
            promote_root(btree,
                         temp_keys[btree->m_min_keys],
                         temp_data[btree->m_min_keys],
                         page_insert,
                         page_sibling);
        }
        else
        {
            // read parent and promote key
            btree_page * parent_page = page_insert->m_parent;

            promote_internal(btree,
                             parent_page,
                             temp_keys[btree->m_min_keys],
                             temp_data[btree->m_min_keys],
                             page_sibling);
        }

        // release resources
        free(temp_keys);
        free(temp_data);
        free(temp_links);
    }
    else
    {
        int insert_index = 0;

        // find insertion point
        while ((insert_index < page_insert->m_key_count) && (page_insert->m_keys[insert_index] < key))
            ++insert_index;

        // shift keys right
        for (int n = page_insert->m_key_count; n > insert_index; --n)
        {
            page_insert->m_keys[n]      = page_insert->m_keys[n - 1];
            page_insert->m_data[n]      = page_insert->m_data[n - 1];
            page_insert->m_links[n + 1] = page_insert->m_links[n];
        }

        page_insert->m_keys[insert_index]      = key;
        page_insert->m_data[insert_index]      = datum;
        page_insert->m_links[insert_index + 1] = link;

        ++page_insert->m_key_count;
    }
}

// internal function to write keys
static void write_key(btree * btree,
                      search_result * insert_info,
                      btree_key_t key,
                      btree_data_t datum)
{
    // check to see if page is full
    if (insert_info->m_page->m_key_count == btree->m_order)
    {
        // temporary arrays
        btree_key_t  * temp_keys = (btree_key_t *)malloc(sizeof(btree_key_t) * (btree->m_order + 1));
        btree_data_t * temp_data = (btree_data_t *)malloc(sizeof(btree_data_t) * (btree->m_order + 1));

        // store new item
        temp_keys[insert_info->m_index] = key;
        temp_data[insert_info->m_index] = datum;

        // copy entries from insertion page to temps
        // TODO: change these to use pointers instead of indexes?
        int nt = 0; // index for temp arrays
        int ni = 0; // index for insertion page

        while (ni < btree->m_order)
        {
            // skip over inserted data
            if (ni == insert_info->m_index)
                ++nt;

            // copy data
            temp_keys[nt] = insert_info->m_page->m_keys[ni];
            temp_data[nt] = insert_info->m_page->m_data[ni];

            // next one
            ++ni;
            ++nt;
        }

        // create a new leaf
        btree_page * page_sibling = alloc_page(btree);

        // assign parent        
        page_sibling->m_parent = insert_info->m_page->m_parent;

        // clear key counts
        insert_info->m_page->m_key_count = 0;
        page_sibling->m_key_count        = 0;

        // copy keys from temp to pages
        for (ni = 0; ni < btree->m_min_keys; ++ni)
        {
            insert_info->m_page->m_keys[ni] = temp_keys[ni];
            insert_info->m_page->m_data[ni] = temp_data[ni];

            ++insert_info->m_page->m_key_count;
        }

        for (ni = btree->m_min_keys + 1; ni <= btree->m_order; ++ni)
        {
            page_sibling->m_keys[ni - 1 - btree->m_min_keys] = temp_keys[ni];
            page_sibling->m_data[ni - 1 - btree->m_min_keys] = temp_data[ni];

            ++page_sibling->m_key_count;
        }

        // replace remaining entries with null
        for (ni = btree->m_min_keys; ni < btree->m_order; ++ni)
        {
            insert_info->m_page->m_keys[ni] = NULL_KEY;
            insert_info->m_page->m_data[ni] = NULL_DATA;
        }

        // promote key and its pointer
        if (insert_info->m_page->m_parent == NULL)
        {
            // creating a new root page
            promote_root(btree,
                         temp_keys[btree->m_min_keys],
                         temp_data[btree->m_min_keys],
                         insert_info->m_page,
                         page_sibling);
        }
        else
        {
            // read parent
            btree_page * page_parent = insert_info->m_page->m_parent;
            
            // promote key into parent page
            promote_internal(btree,
                             page_parent,
                             temp_keys[btree->m_min_keys],
                             temp_data[btree->m_min_keys],
                             page_sibling);
        }
        
        // release sibling page
        free(temp_keys);
        free(temp_data);
    }
    else
    {
        // move keys to make room for new one
        for (int n = insert_info->m_page->m_key_count; n > insert_info->m_index; --n)
        {
            insert_info->m_page->m_keys[n] = insert_info->m_page->m_keys[n - 1];
            insert_info->m_page->m_data[n] = insert_info->m_page->m_data[n - 1];
        }

        insert_info->m_page->m_keys[insert_info->m_index] = key;
        insert_info->m_page->m_data[insert_info->m_index] = datum;

        ++insert_info->m_page->m_key_count;
    }
}

bool btree_insert(btree * btree, btree_key_t key, btree_data_t datum)
{
    // what we return
    bool result = false;
    
    // make sure the arguments make sense
    if (btree != NULL)
    {
        // search for the key
        btree_page * root = btree->m_root;
        
        if (root != NULL)
        {
            search_result insert_info = search(btree,root,key);
            write_key(btree,&insert_info,key,datum);
            ++btree->m_count;
            ++btree->m_ticker;
            result = true;
        }
    }
    
    // done    
    return result;
}

btree_data_t btree_find(btree * btree, btree_key_t key)
{
    // what we return
    btree_data_t result = NULL_DATA;
    
    // make sure the arguments make sense
    if (btree != NULL)
    {
        // search for the key
        btree_page * root = btree->m_root;
        
        if (root != NULL)
        {
            search_result find_info = search(btree,root,key);

            if (find_info.m_found)
                result = find_info.m_page->m_data[find_info.m_index];
        }
    }
    
    // done    
    return result;
}

static void redistribute(btree * btree, int index, btree_page * page_before, btree_page * page_parent, btree_page * page_after)
{
    if ((btree != NULL) && (page_before != NULL) && (page_parent != NULL) && (page_after != NULL))
    {
        // check for leaf page
        if (page_before->m_links[0] == NULL)
        {
            if (page_before->m_key_count > page_after->m_key_count)
            {
                // move a key from page_before to page_after
                for (int n = page_after->m_key_count; n > 0; --n)
                {
                    page_after->m_keys[n] = page_after->m_keys[n - 1];
                    page_after->m_data[n] = page_after->m_data[n - 1];
                }

                // store parent separator in page_after
                page_after->m_keys[0] = page_parent->m_keys[index];
                page_after->m_data[0] = page_parent->m_data[index];

                // increment page_after key count
                ++page_after->m_key_count;

                // decrement page_before key count
                --page_before->m_key_count;

                // move last key in page_before to page_parent as separator
                page_parent->m_keys[index] = page_before->m_keys[page_before->m_key_count];
                page_parent->m_data[index] = page_before->m_data[page_before->m_key_count];

                // clear last key in page_before
                page_before->m_keys[page_before->m_key_count] = NULL_KEY;
                page_before->m_data[page_before->m_key_count] = NULL_DATA;
            }
            else
            {
                // add parent key to lesser page
                page_before->m_keys[page_before->m_key_count] = page_parent->m_keys[index];
                page_before->m_data[page_before->m_key_count] = page_parent->m_data[index];
                
                // increment page_before key count
                ++page_before->m_key_count;

                // move first key in page_after to page_parent as separator
                page_parent->m_keys[index] = page_after->m_keys[0];
                page_parent->m_data[index] = page_after->m_data[0];

                // decrement page_after key count
                --page_after->m_key_count;

                // move a key from page_after to page_before
                int n;
                
                for (n = 0; n < page_after->m_key_count; ++n)
                {
                    page_after->m_keys[n] = page_after->m_keys[n + 1];
                    page_after->m_data[n] = page_after->m_data[n + 1];
                }

                // clear last key in page_after
                page_after->m_keys[n] = NULL_KEY;
                page_after->m_data[n] = NULL_DATA;
            }
        }
        else
        {
            if (page_before->m_key_count > page_after->m_key_count)
            {
                // move a key from page_before to page_after
                for (int n = page_after->m_key_count; n > 0; --n)
                {
                    page_after->m_keys[n]      = page_after->m_keys[n - 1];
                    page_after->m_data[n]      = page_after->m_data[n - 1];
                    page_after->m_links[n + 1] = page_after->m_links[n];
                }

                page_after->m_links[1] = page_after->m_links[0];

                // store page_parent separator key in page_after
                page_after->m_keys[0]  = page_parent->m_keys[index];
                page_after->m_data[0]  = page_parent->m_data[index];
                page_after->m_links[0] = page_before->m_links[page_before->m_key_count];

                // update child link
                btree_page * page_child = page_after->m_links[0];

                if (page_child != NULL)
                    page_child->m_parent = page_after;
                    // TODO: should be an error handler here:

                // increment page_after key count
                ++page_after->m_key_count;

                // decrement page_before key count
                --page_before->m_key_count;

                // move last key in page_before to page_parent as separator
                page_parent->m_keys[index] = page_before->m_keys[page_before->m_key_count];
                page_parent->m_data[index] = page_before->m_data[page_before->m_key_count];

                // clear last key in page_before
                page_before->m_keys[page_before->m_key_count]      = NULL_KEY;
                page_before->m_data[page_before->m_key_count]      = NULL_DATA;
                page_before->m_links[page_before->m_key_count + 1] = NULL;
            }
            else
            {
                // store page_parent separator key in page_before
                page_before->m_keys[page_before->m_key_count]      = page_parent->m_keys[index];
                page_before->m_data[page_before->m_key_count]      = page_parent->m_data[index];
                page_before->m_links[page_before->m_key_count + 1] = page_after->m_links[0];

                // update child link
                btree_page * page_child = page_after->m_links[0];

                if (page_child != NULL)
                    page_child->m_parent = page_before;
                else
                {
                    // TODO: should be an error handler here:
                }

                // increment page_before key count
                ++page_before->m_key_count;

                // move last key in page_after to page_parent as separator
                page_parent->m_keys[index] = page_after->m_keys[0];
                page_parent->m_data[index] = page_after->m_data[0];

                // decrement page_after key count
                --page_after->m_key_count;

                // move a key from page_after to page_before
                int n;

                for (n = 0; n < page_after->m_key_count; ++n)
                {
                    page_after->m_keys[n]  = page_after->m_keys[n + 1];
                    page_after->m_data[n]  = page_after->m_data[n + 1];
                    page_after->m_links[n] = page_after->m_links[n + 1];
                }

                page_after->m_links[n] = page_after->m_links[n + 1];

                // clear last key in page_after
                page_after->m_keys[n]      = NULL_KEY;
                page_after->m_data[n]      = NULL_DATA;
                page_after->m_links[n + 1] = NULL;
            }
        }
    }
}

// prototype for concatenate
static void adjust_tree(btree * btree, btree_page * page);

static void concatenate(btree * btree, int index, btree_page * page_before, btree_page * page_parent, btree_page * page_after)
{
    if ((btree != NULL) && (page_before != NULL) && (page_parent != NULL) && (page_after != NULL))
    {
        // move separator key from page_parent into page_before
        page_before->m_keys[page_before->m_key_count]      = page_parent->m_keys[index];
        page_before->m_data[page_before->m_key_count]      = page_parent->m_data[index];
        page_before->m_links[page_before->m_key_count + 1] = page_after->m_links[0];

        // increment page_before key count
        ++page_before->m_key_count;

        // delete separator from page_parent
        --page_parent->m_key_count;

        int n;

        for (n = index; n < page_parent->m_key_count; ++n)
        {
            page_parent->m_keys[n]      = page_parent->m_keys[n + 1];
            page_parent->m_data[n]      = page_parent->m_data[n + 1];
            page_parent->m_links[n + 1] = page_parent->m_links[n + 2];
        }

        // clear unused key from parent
        page_parent->m_keys[n]      = NULL_KEY;
        page_parent->m_data[n]      = NULL_DATA;
        page_parent->m_links[n + 1] = NULL;

        // copy keys from page_after to page_before
        int n2 = 0;
		n = page_before->m_key_count;

		// combine pages
		while (n2 < page_after->m_key_count)
		{
		    ++page_before->m_key_count;

		    page_before->m_keys[n]      = page_after->m_keys[n2];
		    page_before->m_data[n]      = page_after->m_data[n2];
		    page_before->m_links[n + 1] = page_after->m_links[n2 + 1];

		    ++n2;
		    ++n;
		}

		// delete page_after
        free_page(page_after);

        // is this an inner page?
        if (page_before->m_links[0] != NULL)
        {
            // adjust child pointers
            for (n = 0; n <= page_before->m_key_count; ++n)
            {
                btree_page * page_child = page_before->m_links[n];

                if (page_child != NULL)
                    page_child->m_parent = page_before;
                
                // TODO: error handling?
            }
        }

        // write page_before and parent
        if (page_parent->m_key_count == 0)
        {
            // remove empty parent page
            free_page(page_parent);
            
            // read parents's parent
            btree_page * parents_parent = page_parent->m_parent;
            
            // find parent reference and replace with before page
            if (parents_parent != NULL)
            {
                for (n = 0; n < btree->m_links_size; ++n)
                {
                    if (parents_parent->m_links[n] == page_parent)
                        parents_parent->m_links[n] = page_before;
                }
            }

            // update before page with old parent's parent
            page_before->m_parent = page_parent->m_parent;
            
            if (page_before->m_parent == NULL)
                btree->m_root = page_before;
        }
        else
        {
            // reset root page if needed
            if (page_parent->m_parent == NULL)
                btree->m_root = page_parent;

            // if parent is too small, adjust
            if (page_parent->m_key_count < btree->m_min_keys)
                adjust_tree(btree,page_parent);
        }
    }
}

static void adjust_tree(btree * btree, btree_page * page)
{
    if ((btree != NULL) && (page != NULL) && (page->m_parent != NULL))
    {
        // get parent page
        btree_page * page_parent = page->m_parent;
        
        if (page_parent != NULL)
        {
            // find pointer to page
            int n = 0;

            while (page_parent->m_links[n] != page)
                ++n;

            // read sibling pages
            btree_page * page_sibling_after  = NULL;
            btree_page * page_sibling_before = NULL;

            if (n < page_parent->m_key_count)
                page_sibling_after = page_parent->m_links[n+1];

            if (n > 0)
                page_sibling_before = page_parent->m_links[n-1];

            // figure out what to do
            if (page_sibling_before != NULL)
            {
                --n;

                if (page_sibling_before->m_key_count > btree->m_min_keys)
                    redistribute(btree,n,page_sibling_before,page_parent,page);
                else
                    concatenate(btree,n,page_sibling_before,page_parent,page);
            }
            else
            {
                if (page_sibling_after != NULL)
                {
                    if (page_sibling_after->m_key_count > btree->m_min_keys)
                        redistribute(btree,n,page,page_parent,page_sibling_after);
                    else
                        concatenate(btree,n,page,page_parent,page_sibling_after);
                }
            }
        }
    }
}

bool btree_remove(btree * btree, btree_key_t key)
{
    // what we return
    bool result = false;
    
    // make sure the arguments make sense
    if (btree != NULL)
    {
        // search for the key
        btree_page * root = btree->m_root;
        
        if (root != NULL)
        {
            // search for key
            search_result remove_info = search(btree,root,key);

            if (remove_info.m_found)
            {
                // is this a leaf node?
                if (remove_info.m_page->m_links[0] == NULL)
                {
                    // removing key from leaf
                    --remove_info.m_page->m_key_count;

                    // slide keys left over removed one
                    for (int n = remove_info.m_index; n < remove_info.m_page->m_key_count; ++n)
                    {
                        remove_info.m_page->m_keys[n] = remove_info.m_page->m_keys[n + 1];
                        remove_info.m_page->m_data[n] = remove_info.m_page->m_data[n + 1];
                    }

                    remove_info.m_page->m_keys[remove_info.m_page->m_key_count] = NULL_KEY;
                    remove_info.m_page->m_data[remove_info.m_page->m_key_count] = NULL_DATA;

                    // adjust the tree, if needed
                    if (remove_info.m_page->m_key_count < btree->m_min_keys)
                        adjust_tree(btree,remove_info.m_page);
                    
                    result = true;
                }
                else // removing from an internal page
                {
                    // get the successor page
                    btree_page * page_successor = remove_info.m_page->m_links[remove_info.m_index + 1];

                    if (page_successor != NULL)
                    {
                        while (page_successor->m_links[0] != NULL)
                            page_successor = page_successor->m_links[0];
                        
                        // first key is the "swappee"
                        remove_info.m_page->m_keys[remove_info.m_index] = page_successor->m_keys[0];
                        remove_info.m_page->m_data[remove_info.m_index] = page_successor->m_data[0];

                        // remove swapped key from successor page
                        --page_successor->m_key_count;

                        for (int n = 0; n < page_successor->m_key_count; ++n)
                        {
                            page_successor->m_keys[n]      = page_successor->m_keys[n + 1];
                            page_successor->m_data[n]      = page_successor->m_data[n + 1];
                            page_successor->m_links[n + 1] = page_successor->m_links[n + 2];
                        }

                        page_successor->m_keys[page_successor->m_key_count]      = NULL_KEY;
                        page_successor->m_data[page_successor->m_key_count]      = NULL_DATA;
                        page_successor->m_links[page_successor->m_key_count + 1] = NULL;

                        // adjust tree for leaf node
                        if (page_successor->m_key_count < btree->m_min_keys)
                            adjust_tree(btree,page_successor);
                        
                        // free successor page
                        result = true;
                    }
                }

                --btree->m_count;
            }
        }
    }
    
    // done    
    return result;
}

// embedded random number generator; ala Park and Miller
static       int32_t seed = 1325;
static const int32_t IA   = 16807;
static const int32_t IM   = 2147483647;
static const int32_t IQ   = 127773;
static const int32_t IR   = 2836;
static const int32_t MASK = 123459876;

// return number within a given range
static btree_key_t random_key(btree_key_t limit)
{
    int32_t k;
    int32_t result;
    
    seed ^= MASK;
    k = seed / IQ;
    seed = IA * (seed - k * IQ) - IR * k;
    
    if (seed < 0L)
        seed += IM;
    
    result = (seed % limit);
    seed ^= MASK;
    
    return result;
}

int main(int argc, char ** argv)
{
    int i;
    
    // do we have verbose output?
    bool ga_testing = false;
    
    if (argc > 1)
    {
        for (i = 1; i < argc; ++i)
        {
            if (!strcmp(argv[1],"-ga"))
            {
                ga_testing = true;
                break;
            }
        }
    }
    
    // initialize
    btree * tree = create_btree(16);
    
    bool flags[MAX_KEY];
    
    for (i = 0; i < MAX_KEY; ++i)
        flags[i] = false;
    
    // get starting time    
    struct timespec start, stop;
    clock_gettime(CLOCK_REALTIME,&start);

    // what we're timing
    for (int n = 0; n < TEST_SIZE; ++n)
    {
        // pick a key
        btree_key_t key = random_key(MAX_KEY);
        
        // is the key in the tree?
        btree_data_t data = btree_find(tree,key);
        
        if (data == NULL_DATA)
        {
            btree_insert(tree,key,(btree_data_t)key);
            flags[key] = true;
        }
        else
        {
            btree_remove(tree,key);
            flags[key] = false;
        }
    }
    
    // calculate run time
    clock_gettime(CLOCK_REALTIME,&stop);        
    double run_time = (stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    
#if defined(VERIFY)
    // verify
    for (btree_key_t k = 0; k < MAX_KEY; ++k)
    {
        if (NULL_DATA == btree_find(tree,k))
        {
            if (flags[k])
                fprintf(stderr,"VERIFICATION ERROR: %l found, and shouldn't have been\n",k);
        }
        else
        {
            if (!flags[k])
                fprintf(stderr,"VERIFICATION ERROR: %l not found, and should have been\n",k);
        }
    }
#endif
    
    // clean up
    free_btree(tree);

    // report runtime
    if (ga_testing)
        fprintf(stdout,"%f",run_time);
    else        
        fprintf(stdout,"\ntreebench (Std. C) run time: %f\n\n",run_time);
    
    fflush(stdout);
    
    // done
    return 0;
}
