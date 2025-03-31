/*Convert a word into a linked list, and allow user to change the individual letters
  and add letters as need be*/

#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "functions.h"

using namespace std;

int main()
{
    system("clear");
    
    Node *main_head, *t; //t is to test if new head node is being returned, in which case that is now the new head node
    
    string word;// = "Hello world!"; 
    int mode = 0;

    cout << "Enter WORD:" << endl;
    getline(cin,word); //use getline because cin stops at first whitespace
    main_head = convertWord(word);

    
    bool running = true;
    while (running)
    {

        int index = 0;
        string c;
        t = NULL;
        //t = removeNode(ind, main_head);

        switch (mode)
        {
            default: // get choice

                mode = menuInput(main_head);
                continue;

            case 1: //insert mode

                system("clear");
                displayLL(main_head);

                cin.ignore(); //clear input stream                
                cout << "Enter character(leave blank to exit back to menu): " << endl;
                getline(cin,c);
                if (c.empty())
                {
                    mode = 0;
                    continue;
                }

                cout << "Enter index where character is to be inserted: " << endl;
                //cin.ignore();
                cin >> index;

                t = insertNode(index,c[0],main_head);
                if ( t != NULL)
                {
                    main_head = t;
                }

                break;

            case 2: //remove node

                system("clear");
                displayLL(main_head);

                cin.ignore();
                cout << "Enter index of node to delete(leave blank to exit) " << endl;
                getline(cin,c);
                if (c.empty())
                {
                    mode = 0;
                    continue;
                }

                index = stoi(c);
                t = removeNode(index,main_head);
                if (t != NULL)
                {
                    main_head = t;
                }

                break;

            case 3:

                running = false;
                break;
        }
    }

    cout << "\n END OF PROGRAM. \n";
    return 0;
}    