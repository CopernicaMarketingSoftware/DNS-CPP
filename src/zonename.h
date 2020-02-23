/**
 *  ZoneName.h
 * 
 *  This class is used to convert a domain name into a zonename. For
 *  example the zonename of www.example.com is example.com, etcetera
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class ZoneName
{
private:
    /**
     *  The zone name
     *  @var const char *
     */
    const char *_name;
    
public:
    /**
     *  Constructor
     *  @param  name        the original hostname
     *  @throws std::runtime_error
     */
    ZoneName(const char *name)
    {
        // look for the first dot
        // @todo is it indeed this simple?
        auto *dot = strchr(name, '.');
        
        // should be there
        if (dot == nullptr) throw std::runtime_error("no zone could be extracted");
        
        // we have the zone
        _name = dot + 1;
    }
    
    /**
     *  Destructor
     */
    virtual ~ZoneName() = default;
    
    /**
     *  Compare with a other name
     *  @return bool
     * 
     * 
     *  @todo is this correct?
     */
    bool operator==(const char *name) const
    {
        return strcasecmp(_name, name) == 0;
    }

    /**
     *  Compare with a other name
     *  @return bool
     */
    bool operator!=(const char *name) const
    {
        return !operator==(name);
    }
};
    
/**
 *  End of namespace
 */
}

