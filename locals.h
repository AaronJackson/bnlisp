#ifndef BNLISP_LOCALS_H
#define BNLISP_LOCALS_H

/* as bnl would say, this is
   
   m    #                    #        "
 mm#mm  # mm    mmm          #mmm   mmm     mmmm
   #    #"  #  #"  #         #" "#    #    #" "#
   #    #   #  #""""         #   #    #    #   #
   "mm  #   #  "#mm"         ##m#"  mm#mm  "#m"#
                                            m  #
                                             ""

            #    "             m
         mmm#  mmm     m mm  mm#mm  m   m
        #" "#    #     #"  "   #    "m m"
        #   #    #     #       #     #m#
        "#m##  mm#mm   #       "mm   "#
                                     m"
                                    ""
 
 this file defines some macros for keeping track of
 pointers to lisp objects in C so that they can be properly
 referenced after a garbage collection
 */

/* frame pointer variable name*/
#define FP frame_pointer

#define END_FRAME ((void *) -1)

/* the ugly end of C */

#define LOCALS1(name1)                                  \
  void *locals_frame_[3] = {};                          \
  obj_t **name1 = (obj_t **)(locals_frame_ + 1);        \
  locals_frame_[0] = FP;                                \
  locals_frame_[2] = END_FRAME;                         \
  FP = locals_frame_;

#define LOCALS2(name1, name2)                           \
  void *locals_frame_[4] = {};                          \
  obj_t **name1 = (obj_t **)(locals_frame_ + 1);        \
  obj_t **name2 = (obj_t **)(locals_frame_ + 2);        \
  locals_frame_[0] = FP;                                \
  locals_frame_[3] = END_FRAME;                         \
  FP = locals_frame_;

#define LOCALS3(name1, name2, name3)                    \
  void *locals_frame_[5] = {};                          \
  obj_t **name1 = (obj_t **)(locals_frame_ + 1);        \
  obj_t **name2 = (obj_t **)(locals_frame_ + 2);        \
  obj_t **name3 = (obj_t **)(locals_frame_ + 3);        \
  locals_frame_[0] = FP;                                \
  locals_frame_[4] = END_FRAME;                         \
  FP = locals_frame_;

#define LOCALS4(name1, name2, name3, name4)             \
  void *locals_frame_[6] = {};                          \
  obj_t **name1 = (obj_t **)(locals_frame_ + 1);        \
  obj_t **name2 = (obj_t **)(locals_frame_ + 2);        \
  obj_t **name3 = (obj_t **)(locals_frame_ + 3);        \
  obj_t **name4 = (obj_t **)(locals_frame_ + 4);        \
  locals_frame_[0] = FP;                                \
  locals_frame_[5] = END_FRAME;                         \
  FP = locals_frame_;

#endif
