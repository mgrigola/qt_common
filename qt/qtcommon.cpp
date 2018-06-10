#include "QtCommon.h"

//returns true if item was changed. Else returns false
bool Add_To_ComboBox_History(QComboBox* pComboBox, QString stringToAdd)
{
    int existingPos = pComboBox->findText(stringToAdd);

    //item not in list -> add it
    if (existingPos == -1)
    {
        pComboBox->addItem(stringToAdd);
        return true;
    }
    //item already in list, but not at end, so move to end // ### i'm making end the most recent. definitely backwards, and i probably shouldn't have this code because same is already built into the combo box (why this always gets called twice...), but whatever
    else if (existingPos != (pComboBox->count() - 1) )
    {
        pComboBox->removeItem(existingPos);
        pComboBox->addItem(stringToAdd);
        //pComboBox->setCurrentIndex(pComboBox->count()-1);  //this might make infinite loop...
        pComboBox->setCurrentText( stringToAdd );  //not necessary? does it trigger our source signal again in an endless loop? //negative, absolutely necessary. doesn't seem to loop...
        return true;
    }
    //else item is already at end of list -> do nothing
    return false;
}
