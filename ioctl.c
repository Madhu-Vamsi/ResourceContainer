//////////////////////////////////////////////////////////////////////
//                      North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng, Yu-Chia Liu
//
//   Description:
//     Core of Kernel Module for Processor Container
//
////////////////////////////////////////////////////////////////////////

#include "processor_container.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kthread.h>

// Project 1: Shikhar Sharma,ssharm29; Madhu Machavarapu, mmachav;

struct thread_object{                                      
    struct task_struct* tsk;
    struct thread_object* t_next;
};

 struct container_object{
    long long unsigned int cid;
    struct container_object* c_next;
    struct thread_object* t_head;
};

typedef struct container_object c_node;
typedef struct thread_object t_node;

c_node* c_head=NULL;
extern struct mutex lock;

int processor_container_delete(struct processor_container_cmd __user *user_cmd)
{
    mutex_lock(&lock);
    struct processor_container_cmd temp;
    copy_from_user(&temp,user_cmd,sizeof(struct processor_container_cmd));
    c_node* c_now=c_head;
    c_node *c_prev=NULL;
    while(c_now!=NULL){
        if(c_now->cid==temp.cid){
            t_node *t_rem=c_now->t_head;
            c_now->t_head=t_rem->t_next;
            kfree(t_rem);
            if(c_now->t_head!=NULL)
            {
                wake_up_process(c_now->t_head->tsk);
                mutex_unlock(&lock);
                return 0;
            }
            else if(c_prev!=NULL)
            {
                c_prev->c_next=c_now->c_next;
                kfree(c_now);
            }
            else
            {
                c_head=c_now->c_next;
                kfree(c_now);
            }
        }
        c_prev=c_now;
        c_now=c_now->c_next;
    }
    mutex_unlock(&lock);
    return 0;
}

int processor_container_create(struct processor_container_cmd __user *user_cmd)
{
    mutex_lock(&lock);
    struct processor_container_cmd temp;
    copy_from_user(&temp,user_cmd,sizeof(struct processor_container_cmd));
    if(c_head==NULL){
        c_node* new_head=(c_node *)kmalloc(sizeof(c_node),GFP_KERNEL);
        new_head->cid=temp.cid;
        new_head->c_next=NULL;
        t_node* t_new=(t_node*)kmalloc(sizeof(t_node),GFP_KERNEL);
        t_new->tsk=current;
        t_new->t_next=NULL;
        new_head->t_head=t_new;
        c_head=new_head;
    }
    else{
        c_node *c_now=c_head;

            while(c_now!=NULL){
            if(c_now->cid==temp.cid){
                t_node *t_now=c_now->t_head;
                while(t_now->t_next!=NULL)
                        t_now=t_now->t_next;
                t_node* t_new=(t_node*)kmalloc(sizeof(t_node),GFP_KERNEL);
                t_new->tsk=current;
                t_new->t_next=NULL;   
                t_now->t_next=t_new;
                set_current_state(TASK_INTERRUPTIBLE);
                mutex_unlock(&lock);
                schedule();
                return 0;
            }
            c_now = c_now->c_next;
        }
        c_now=c_head;
        while(c_now->c_next!=NULL){
            c_now=c_now->c_next;
        }
        c_node* c_new = (c_node*) kmalloc(sizeof(c_node),GFP_KERNEL);
        t_node* t_new=(t_node*)kmalloc(sizeof(t_node),GFP_KERNEL);
        t_new->tsk=current;
        t_new->t_next=NULL;  
        c_new->t_head=t_new;
        c_now->c_next=c_new;
        c_new->c_next=NULL;
        c_new->cid=temp.cid;
    }    
     mutex_unlock(&lock);
    return 0;
}

int processor_container_switch(struct processor_container_cmd __user *user_cmd)
{

    mutex_lock(&lock);
    c_node* fakeHead=c_head;
    while(fakeHead!=NULL){
        if(fakeHead->t_head->tsk==current){
            if(fakeHead->t_head->t_next==NULL)
                goto down;
            t_node* to_be_slept=fakeHead->t_head;
            t_node* to_be_woken_up=fakeHead->t_head->t_next;
            fakeHead->t_head=to_be_woken_up;
            t_node* tmp=to_be_woken_up;
            while(tmp->t_next!=NULL)
                tmp=tmp->t_next;
            tmp->t_next=to_be_slept;
            to_be_slept->t_next=NULL;
            set_current_state(TASK_INTERRUPTIBLE);
            wake_up_process(to_be_woken_up->tsk);
            mutex_unlock(&lock);
            schedule();
            break;
        }
        fakeHead=fakeHead->c_next;
    }
        down:mutex_unlock(&lock);
        return 0;
}

int processor_container_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg)
{
    switch (cmd)
    {
    case PCONTAINER_IOCTL_CSWITCH:
        return processor_container_switch((void __user *)arg);
    case PCONTAINER_IOCTL_CREATE:
        return processor_container_create((void __user *)arg);
    case PCONTAINER_IOCTL_DELETE:
        return processor_container_delete((void __user *)arg);
    default:
        return -ENOTTY;
    }
}