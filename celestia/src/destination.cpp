// destination.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include "celestia.h"
#include "util.h"
#include "parser.h"
#include "destination.h"

using namespace std;


Destination::Destination() :
    name(""),
    target(""),
    distance(0.0),
    description("")
{
}


DestinationList* ReadDestinationList(istream& in)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);
    DestinationList* destinations = new DestinationList();

    cout << "ReadDestinationList\n";

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenBeginGroup)
        {
            DPRINTF("Error parsing destinations file.\n");
            for_each(destinations->begin(), destinations->end(), deleteFunc<Destination*>());
            delete destinations;
            return NULL;
        }
        tokenizer.pushBack();

        Value* destValue = parser.readValue();
        if (destValue == NULL || destValue->getType() != Value::HashType)
        {
            DPRINTF("Error parsing destination.\n");
            for_each(destinations->begin(), destinations->end(), deleteFunc<Destination*>());
            delete destinations;
            if (destValue != NULL)
                delete destValue;
            return NULL;
        }

        Hash* destParams = destValue->getHash();
        Destination* dest = new Destination();
        
        if (!destParams->getString("Name", dest->name))
        {
            DPRINTF("Skipping unnamed destination\n");
            delete dest;
        }
        else
        {
            cout << "Reading destination: " << dest->name << '\n';
            destParams->getString("Target", dest->target);
            destParams->getString("Description", dest->description);
            destParams->getNumber("Distance", dest->distance);
            destinations->insert(destinations->end(), dest);
        }

        delete destValue;
    }

    return destinations;
}
