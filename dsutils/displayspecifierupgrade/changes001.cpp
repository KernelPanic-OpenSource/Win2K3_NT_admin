// This file is generated by W2KRepl.exe
// Copyright (c) 2001 Microsoft Corporation
// Jun 2001 lucios


#include "headers.hxx"
#include "constants.hpp"



void setChanges1()
{
   // the next three changes fix bug #497488
   addChange
   (
      guids[1],
      0x410,
      L"serversContainer-Display",
      L"classDisplayName",
      //Contenitore Server 
      L"\x43\x6f\x6e\x74\x65\x6e\x69\x74\x6f\x72\x65\x20\x53\x65\x72\x76\x65\x72",
      //Contenitore server 
      L"\x43\x6f\x6e\x74\x65\x6e\x69\x74\x6f\x72\x65\x20\x73\x65\x72\x76\x65\x72",
      REPLACE_W2K_SINGLE_VALUE
   );


   addChange
   (
      guids[1],
      0x413,
      L"group-Display",
      L"classDisplayName",
      //Groep 
      L"\x47\x72\x6f\x65\x70",
      //GROEP 
      L"\x47\x52\x4f\x45\x50",
      REPLACE_W2K_SINGLE_VALUE
   );

   addChange
   (
      guids[1],
      0x413,
      L"domainDNS-Display",
      L"classDisplayName",
      //Domein 
      L"\x44\x6f\x6d\x65\x69\x6e",
      //domein 
      L"\x64\x6f\x6d\x65\x69\x6e",
      REPLACE_W2K_SINGLE_VALUE
   );

    // The remaining changes are a fix for bug 517587 and for Korean localization bugs
   // 505060, 505065 and 510733
    addChange
    (
        guids[1],
        0x409,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, International ISDN Number (Others)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x49\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x20\x49\x53\x44\x4e\x20\x4e\x75\x6d\x62\x65\x72\x20\x28\x4f\x74\x68\x65\x72\x73\x29",
        //internationalISDNNumber,International ISDN Number (Others)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x49\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x20\x49\x53\x44\x4e\x20\x4e\x75\x6d\x62\x65\x72\x20\x28\x4f\x74\x68\x65\x72\x73\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x401,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, ??? ISDN ?????? (???)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x631\x642\x645\x20\x49\x53\x44\x4e\x20\x627\x644\x62f\x648\x644\x64a\x20\x28\x622\x62e\x631\x29",
        //internationalISDNNumber,??? ISDN ?????? (???)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x631\x642\x645\x20\x49\x53\x44\x4e\x20\x627\x644\x62f\x648\x644\x64a\x20\x28\x622\x62e\x631\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x405,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, Mezin�rodn� c�slo ISDN (ostatn�)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x4d\x65\x7a\x69\x6e\xe1\x72\x6f\x64\x6e\xed\x20\x10d\xed\x73\x6c\x6f\x20\x49\x53\x44\x4e\x20\x28\x6f\x73\x74\x61\x74\x6e\xed\x29",
        //internationalISDNNumber,Mezin�rodn� c�slo ISDN (ostatn�)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x4d\x65\x7a\x69\x6e\xe1\x72\x6f\x64\x6e\xed\x20\x10d\xed\x73\x6c\x6f\x20\x49\x53\x44\x4e\x20\x28\x6f\x73\x74\x61\x74\x6e\xed\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x408,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, ??e???? a???�?? ISDN (?????)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x394\x3b9\x3b5\x3b8\x3bd\x3ae\x3c2\x20\x3b1\x3c1\x3b9\x3b8\x3bc\x3cc\x3c2\x20\x49\x53\x44\x4e\x20\x28\x386\x3bb\x3bb\x3bf\x3b9\x29",
        //internationalISDNNumber,??e???? a???�?? ISDN (?????)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x394\x3b9\x3b5\x3b8\x3bd\x3ae\x3c2\x20\x3b1\x3c1\x3b9\x3b8\x3bc\x3cc\x3c2\x20\x49\x53\x44\x4e\x20\x28\x386\x3bb\x3bb\x3bf\x3b9\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x40c,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, Num�ro RNIS international (Autres)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x4e\x75\x6d\xe9\x72\x6f\x20\x52\x4e\x49\x53\x20\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x20\x28\x41\x75\x74\x72\x65\x73\x29",
        //internationalISDNNumber,Num�ro RNIS international (Autres)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x4e\x75\x6d\xe9\x72\x6f\x20\x52\x4e\x49\x53\x20\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x20\x28\x41\x75\x74\x72\x65\x73\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x40d,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, ???? ISDN ???????? (?????)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x5de\x5e1\x5e4\x5e8\x20\x49\x53\x44\x4e\x20\x5d1\x5d9\x5e0\x5dc\x5d0\x5d5\x5de\x5d9\x20\x28\x5d0\x5d7\x5e8\x5d9\x5dd\x29",
        //internationalISDNNumber,???? ISDN ???????? (?????)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x5de\x5e1\x5e4\x5e8\x20\x49\x53\x44\x4e\x20\x5d1\x5d9\x5e0\x5dc\x5d0\x5d5\x5de\x5d9\x20\x28\x5d0\x5d7\x5e8\x5d9\x5dd\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x40e,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, Nemzetk�zi ISDN-sz�m (egy�b)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x4e\x65\x6d\x7a\x65\x74\x6b\xf6\x7a\x69\x20\x49\x53\x44\x4e\x2d\x73\x7a\xe1\x6d\x20\x28\x65\x67\x79\xe9\x62\x29",
        //internationalISDNNumber,Nemzetk�zi ISDN-sz�m (egy�b)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x4e\x65\x6d\x7a\x65\x74\x6b\xf6\x7a\x69\x20\x49\x53\x44\x4e\x2d\x73\x7a\xe1\x6d\x20\x28\x65\x67\x79\xe9\x62\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x410,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, Numero ISDN internazionale (altri)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x4e\x75\x6d\x65\x72\x6f\x20\x49\x53\x44\x4e\x20\x69\x6e\x74\x65\x72\x6e\x61\x7a\x69\x6f\x6e\x61\x6c\x65\x20\x28\x61\x6c\x74\x72\x69\x29",
        //internationalISDNNumber,Numero ISDN internazionale (altri)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x4e\x75\x6d\x65\x72\x6f\x20\x49\x53\x44\x4e\x20\x69\x6e\x74\x65\x72\x6e\x61\x7a\x69\x6f\x6e\x61\x6c\x65\x20\x28\x61\x6c\x74\x72\x69\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x411,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, ?? ISDN ?? (???)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x56fd\x969b\x20\x49\x53\x44\x4e\x20\x756a\x53f7\x20\x28\x305d\x306e\x4ed6\x29",
        //internationalISDNNumber,?? ISDN ?? (???)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x56fd\x969b\x20\x49\x53\x44\x4e\x20\x756a\x53f7\x20\x28\x305d\x306e\x4ed6\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"user-Display",
        L"attributeDisplayNames",
        //givenName,??
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984",
        //givenName,??(? ??)
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984\x28\xc131\x20\xc5c6\xc74c\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"user-Display",
        L"attributeDisplayNames",
        //initials,???
        L"\x69\x6e\x69\x74\x69\x61\x6c\x73\x2c\xc774\xb2c8\xc15c",
        //initials,???(?? ??)
        L"\x69\x6e\x69\x74\x69\x61\x6c\x73\x2c\xc774\xb2c8\xc15c\x28\xc911\xac04\x20\xc774\xb984\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"user-Display",
        L"attributeDisplayNames",
        //personalTitle,??
        L"\x70\x65\x72\x73\x6f\x6e\x61\x6c\x54\x69\x74\x6c\x65\x2c\xc9c1\xd568",
        //personalTitle,??
        L"\x70\x65\x72\x73\x6f\x6e\x61\x6c\x54\x69\x74\x6c\x65\x2c\xd638\xce6d",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"user-Display",
        L"attributeDisplayNames",
        //userWorkstations,?????? ???
        L"\x75\x73\x65\x72\x57\x6f\x72\x6b\x73\x74\x61\x74\x69\x6f\x6e\x73\x2c\xc6cc\xd06c\xc2a4\xd14c\xc774\xc158\x20\xb85c\xadf8\xc628",
        //userWorkstations,??? ??????
        L"\x75\x73\x65\x72\x57\x6f\x72\x6b\x73\x74\x61\x74\x69\x6f\x6e\x73\x2c\xb85c\xadf8\xc628\x20\xc6cc\xd06c\xc2a4\xd14c\xc774\xc158",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"group-Display",
        L"attributeDisplayNames",
        //managedBy,?? ???
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xc7a5\xce58\x20\xad00\xb9ac\xc790",
        //managedBy,??
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xad00\xb9ac",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"contact-Display",
        L"attributeDisplayNames",
        //givenName,??
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984",
        //givenName,??(? ??)
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984\x28\xc131\x20\xc5c6\xc74c\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"contact-Display",
        L"attributeDisplayNames",
        //initials,???
        L"\x69\x6e\x69\x74\x69\x61\x6c\x73\x2c\xc774\xb2c8\xc15c",
        //initials,???(?? ??)
        L"\x69\x6e\x69\x74\x69\x61\x6c\x73\x2c\xc774\xb2c8\xc15c\x28\xc911\xac04\x20\xc774\xb984\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"contact-Display",
        L"attributeDisplayNames",
        //personalTitle,??
        L"\x70\x65\x72\x73\x6f\x6e\x61\x6c\x54\x69\x74\x6c\x65\x2c\xc9c1\xd568",
        //personalTitle,??
        L"\x70\x65\x72\x73\x6f\x6e\x61\x6c\x54\x69\x74\x6c\x65\x2c\xd638\xce6d",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"computer-Display",
        L"attributeDisplayNames",
        //managedBy,?? ???
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xc7a5\xce58\x20\xad00\xb9ac\xc790",
        //managedBy,??
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xad00\xb9ac",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"organizationalUnit-Display",
        L"attributeDisplayNames",
        //managedBy,?? ???
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xc7a5\xce58\x20\xad00\xb9ac\xc790",
        //managedBy,??
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xad00\xb9ac",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"volume-Display",
        L"attributeDisplayNames",
        //managedBy,?? ???
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xc7a5\xce58\x20\xad00\xb9ac\xc790",
        //managedBy,??
        L"\x6d\x61\x6e\x61\x67\x65\x64\x42\x79\x2c\xad00\xb9ac",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"default-Display",
        L"extraColumns",
        //givenName,??,0,100,0
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984\x2c\x30\x2c\x31\x30\x30\x2c\x30",
        //givenName,??(? ??),0,100,0
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984\x28\xc131\x20\xc5c6\xc74c\x29\x2c\x30\x2c\x31\x30\x30\x2c\x30",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"default-Display",
        L"extraColumns",
        //st,State,0,100,0
        L"\x73\x74\x2c\x53\x74\x61\x74\x65\x2c\x30\x2c\x31\x30\x30\x2c\x30",
        //st,?/?,0,100,0
        L"\x73\x74\x2c\xc2dc\x2f\xb3c4\x2c\x30\x2c\x31\x30\x30\x2c\x30",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"lostAndFound-Display",
        L"extraColumns",
        //lastKnownParent,??? ??? ??,1,300,0
        L"\x6c\x61\x73\x74\x4b\x6e\x6f\x77\x6e\x50\x61\x72\x65\x6e\x74\x2c\xb9c8\xc9c0\xb9c9\x20\xc54c\xb824\xc9c4\x20\xbd80\xbaa8\x2c\x31\x2c\x33\x30\x30\x2c\x30",
        //lastKnownParent,????? ??? ??,1,300,0
        L"\x6c\x61\x73\x74\x4b\x6e\x6f\x77\x6e\x50\x61\x72\x65\x6e\x74\x2c\xb9c8\xc9c0\xb9c9\xc73c\xb85c\x20\xc54c\xb824\xc9c4\x20\xbd80\xbaa8\x2c\x31\x2c\x33\x30\x30\x2c\x30",
        REPLACE_W2K_SINGLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //givenName,??
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984",
        //givenName,??(? ??)
        L"\x67\x69\x76\x65\x6e\x4e\x61\x6d\x65\x2c\xc774\xb984\x28\xc131\x20\xc5c6\xc74c\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //initials,???
        L"\x69\x6e\x69\x74\x69\x61\x6c\x73\x2c\xc774\xb2c8\xc15c",
        //initials,???(?? ??)
        L"\x69\x6e\x69\x74\x69\x61\x6c\x73\x2c\xc774\xb2c8\xc15c\x28\xc911\xac04\x20\xc774\xb984\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //personalTitle,??
        L"\x70\x65\x72\x73\x6f\x6e\x61\x6c\x54\x69\x74\x6c\x65\x2c\xc9c1\xd568",
        //personalTitle,??
        L"\x70\x65\x72\x73\x6f\x6e\x61\x6c\x54\x69\x74\x6c\x65\x2c\xd638\xce6d",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x412,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //userWorkstations,?????? ???
        L"\x75\x73\x65\x72\x57\x6f\x72\x6b\x73\x74\x61\x74\x69\x6f\x6e\x73\x2c\xc6cc\xd06c\xc2a4\xd14c\xc774\xc158\x20\xb85c\xadf8\xc628",
        //userWorkstations,??? ??????
        L"\x75\x73\x65\x72\x57\x6f\x72\x6b\x73\x74\x61\x74\x69\x6f\x6e\x73\x2c\xb85c\xadf8\xc628\x20\xc6cc\xd06c\xc2a4\xd14c\xc774\xc158",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x413,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, Internationaal ISDN-nummer (overige)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x49\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x61\x6c\x20\x49\x53\x44\x4e\x2d\x6e\x75\x6d\x6d\x65\x72\x20\x28\x6f\x76\x65\x72\x69\x67\x65\x29",
        //internationalISDNNumber,Internationaal ISDN-nummer (overige)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x49\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x61\x6c\x20\x49\x53\x44\x4e\x2d\x6e\x75\x6d\x6d\x65\x72\x20\x28\x6f\x76\x65\x72\x69\x67\x65\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x414,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, Internasjonalt ISDN-nummber (andre)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x49\x6e\x74\x65\x72\x6e\x61\x73\x6a\x6f\x6e\x61\x6c\x74\x20\x49\x53\x44\x4e\x2d\x6e\x75\x6d\x6d\x62\x65\x72\x20\x28\x61\x6e\x64\x72\x65\x29",
        //internationalISDNNumber,Internasjonalt ISDN-nummer (andre)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x49\x6e\x74\x65\x72\x6e\x61\x73\x6a\x6f\x6e\x61\x6c\x74\x20\x49\x53\x44\x4e\x2d\x6e\x75\x6d\x6d\x65\x72\x20\x28\x61\x6e\x64\x72\x65\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x415,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, Miedzynarodowy numer sieciowy ISDN (inne)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x4d\x69\x119\x64\x7a\x79\x6e\x61\x72\x6f\x64\x6f\x77\x79\x20\x6e\x75\x6d\x65\x72\x20\x73\x69\x65\x63\x69\x6f\x77\x79\x20\x49\x53\x44\x4e\x20\x28\x69\x6e\x6e\x65\x29",
        //internationalISDNNumber,Miedzynarodowy numer sieciowy ISDN (inne)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x4d\x69\x119\x64\x7a\x79\x6e\x61\x72\x6f\x64\x6f\x77\x79\x20\x6e\x75\x6d\x65\x72\x20\x73\x69\x65\x63\x69\x6f\x77\x79\x20\x49\x53\x44\x4e\x20\x28\x69\x6e\x6e\x65\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x416,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, N�mero ISDN internacional (outros)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x4e\xfa\x6d\x65\x72\x6f\x20\x49\x53\x44\x4e\x20\x69\x6e\x74\x65\x72\x6e\x61\x63\x69\x6f\x6e\x61\x6c\x20\x28\x6f\x75\x74\x72\x6f\x73\x29",
        //internationalISDNNumber,N�mero ISDN internacional (outros)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x4e\xfa\x6d\x65\x72\x6f\x20\x49\x53\x44\x4e\x20\x69\x6e\x74\x65\x72\x6e\x61\x63\x69\x6f\x6e\x61\x6c\x20\x28\x6f\x75\x74\x72\x6f\x73\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x419,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, ????????????? ????? ISDN (??????)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x41c\x435\x436\x434\x443\x43d\x430\x440\x43e\x434\x43d\x44b\x439\x20\x43d\x43e\x43c\x435\x440\x20\x49\x53\x44\x4e\x20\x28\x43f\x440\x43e\x447\x438\x435\x29",
        //internationalISDNNumber,????????????? ????? ISDN (??????)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x41c\x435\x436\x434\x443\x43d\x430\x440\x43e\x434\x43d\x44b\x439\x20\x43d\x43e\x43c\x435\x440\x20\x49\x53\x44\x4e\x20\x28\x43f\x440\x43e\x447\x438\x435\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x804,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, ?? ISDN ??(??)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x56fd\x9645\x20\x49\x53\x44\x4e\x20\x53f7\x7801\x28\x5176\x5b83\x29",
        //internationalISDNNumber,?? ISDN ??(??)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x56fd\x9645\x20\x49\x53\x44\x4e\x20\x53f7\x7801\x28\x5176\x5b83\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );


    addChange
    (
        guids[1],
        0x816,
        L"inetOrgPerson-Display",
        L"attributeDisplayNames",
        //internationalISDNNumber, N�mero RDIS internacional (outros)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x20\x4e\xfa\x6d\x65\x72\x6f\x20\x52\x44\x49\x53\x20\x69\x6e\x74\x65\x72\x6e\x61\x63\x69\x6f\x6e\x61\x6c\x20\x28\x6f\x75\x74\x72\x6f\x73\x29",
        //internationalISDNNumber,N�mero RDIS internacional (outros)
        L"\x69\x6e\x74\x65\x72\x6e\x61\x74\x69\x6f\x6e\x61\x6c\x49\x53\x44\x4e\x4e\x75\x6d\x62\x65\x72\x2c\x4e\xfa\x6d\x65\x72\x6f\x20\x52\x44\x49\x53\x20\x69\x6e\x74\x65\x72\x6e\x61\x63\x69\x6f\x6e\x61\x6c\x20\x28\x6f\x75\x74\x72\x6f\x73\x29",
        REPLACE_W2K_MULTIPLE_VALUE
    );
}
